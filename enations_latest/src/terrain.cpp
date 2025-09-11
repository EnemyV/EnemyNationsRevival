//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// terrain.cpp : the hexes & terrain
//
#include "base.h" 
#include "terrain.inl"

#include "bitmaps.h"
#include "bridge.h"
#include "building.inl"
#include "icons.h"
#include "lastplnt.h"
#include "minerals.h"
#include "stdafx.h"
#include "unit.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#define LOG_COST 0

CGameMap         theMap;                   // the world (only one instance)
CTerrain         theTerrain( "terrain" );  // data about the terrain types
CTerrainShowStat tShowStat;

void TerrainStatusText( void* pData, CString& str )
{

    TRAP( );
    CHex* pHex = (CHex*)pData;

    // if no minerals - done
    CMinerals* pMn = NULL;
    if ( ( pHex->GetUnits( ) & CHex::minerals ) != 0 )
        if ( !theMinerals.Lookup( pHex->GetHex( ), pMn ) )
            pMn = NULL;

    // can we show copper?
    if ( ( pMn != NULL ) && ( pMn->GetType( ) == CMaterialTypes::copper ) && ( !theGame.GetMe( )->CanCopper( ) ) )
        pMn = NULL;

    if ( pMn == NULL )
    {
        TRAP( );
        str = "";
        return;
    }

    str.LoadString( IDS_STAT_MINERALS );
    CString sNum1 = IntToCString( pMn->GetQuantity( ) );
    CString sNum2 = IntToCString( pMn->GetDensity( ) );
    csPrintf( &str, (char const*)CMaterialTypes::GetDesc( pMn->GetType( ) ), (char const*)sNum1, (char const*)sNum2 );
}

void CTerrainShowStat::Init( )
{

    m_si[0].Attach( &theIcons, ICON_BAR_TEXT );
    m_si[1].Attach( &theIcons, ICON_MATERIALS );
    m_si[2].Attach( &theIcons, ICON_DENSITY );
}

void CTerrainShowStat::Close( )
{

    for ( int iOn = 0; iOn < 3; iOn++ )
    {
        delete m_si[iOn].m_pDib;
        m_si[iOn].m_pDib  = NULL;
        m_si[iOn].m_rDest = CRect( 0, 0, 0, 0 );
    }
}

void TerrainShowStatus( void* pData, CDC* pDc, CRect const& rDraw, CDIB* pDibBack, CPoint const& ptOff )
{
    CHex* pHex = (CHex*)pData;

    for ( int iOn = 0; iOn < 3; iOn++ ) tShowStat.m_si[iOn].SetBack( pDibBack, ptOff );

    // terrain name
    CRect rect( rDraw );
    int   iWid = 14 * 8 + tShowStat.m_si[0].m_pStatData->m_leftOff + tShowStat.m_si[0].m_pStatData->m_rightOff;
    iWid       = __min( rect.Width( ) / 2, iWid );
    rect.right = rect.left + iWid;
    tShowStat.m_si[0].SetSize( rect );

    tShowStat.m_si[0].SetText( pHex->GetStatus( ) );
    tShowStat.m_si[0].DrawIcon( pDc );

    // if no minerals - done
    CMinerals* pMn = NULL;
    if ( ( pHex->GetUnits( ) & CHex::minerals ) != 0 )
        if ( !theMinerals.Lookup( pHex->GetHex( ), pMn ) )
            pMn = NULL;

    // can we show copper?
    if ( ( pMn != NULL ) && ( pMn->GetType( ) == CMaterialTypes::copper ) && ( !theGame.GetMe( )->CanCopper( ) ) )
        pMn = NULL;

    // quantity left
    rect.left  = rect.right;
    rect.right = rect.left + ( rDraw.right - rect.left ) / 2;
    tShowStat.m_si[1].SetSize( rect );
    tShowStat.m_si[1].SetPerBase( pMn == NULL ? 0 : pMn->GetType( ) );
    if ( ( pMn == NULL ) || ( pMn->GetQuantity( ) == 0 ) )
        tShowStat.m_si[1].SetPer( 0 );
    else

    {
        int iMax = MAX_MINERAL_QUANTITY;
        switch ( pMn->GetType( ) )
        {
        case CMaterialTypes::coal:
            iMax = MAX_MINERAL_COAL_QUANTITY;
            break;
        case CMaterialTypes::iron:
            iMax = MAX_MINERAL_IRON_QUANTITY;
            break;
        case CMaterialTypes::oil:
            iMax = MAX_MINERAL_OIL_QUANTITY;
            break;
        case CMaterialTypes::copper: // lulz Xil
            iMax = MAX_MINERAL_XIL_QUANTITY;
            break;
        }

        tShowStat.m_si[1].SetPer( __max( 1, pMn->GetQuantity( ) / ( iMax / 100 ) ) );
    }
    tShowStat.m_si[1].DrawIcon( pDc );

    // density
    rect.left  = rect.right;
    rect.right = rDraw.right;
    tShowStat.m_si[2].SetSize( rect );
    if ( pMn == NULL )
        tShowStat.m_si[2].SetPer( 0 );
    else
        tShowStat.m_si[2].SetPer( __max( 1, ( 100 * pMn->GetDensity( ) ) / MAX_MINERAL_DENSITY ) );
    tShowStat.m_si[2].DrawIcon( pDc );
}

const CHex& CHex::operator=( CHex const& src )
{

    m_bType    = src.m_bType;
    m_bAlt     = src.m_bAlt;
    m_bUnit    = src.m_bUnit;
    m_bVisible = src.m_bVisible;

    return ( *this );
}


//--------------------------- C B i t M a t r i x --------------------------

//--------------------------------------------------------------------------
// CBitMatrix::CBitMatrix
//--------------------------------------------------------------------------
CBitMatrix::CBitMatrix( int cx, int cy ): m_pby( NULL )
{
    m_iPitchBytes = ( cx + 7 ) >> 3;
    m_nBytes      = cy * m_iPitchBytes;
    m_pby         = new BYTE[m_nBytes];

    ASSERT( 0 < m_iPitchBytes );
    ASSERT( m_nBytes > 0 );
}

//--------------------------------------------------------------------------
// CBitMatrix::~CBitMatrix
//--------------------------------------------------------------------------
CBitMatrix::~CBitMatrix( )
{
    delete[] m_pby;
}

//--------------------------------------------------------------------------
// CBitMatrix::Get
//--------------------------------------------------------------------------
BOOL CBitMatrix::Get( int iX, int iY ) const
{
    int iByte = m_iPitchBytes * iY + ( iX >> 3 );
    int iMask = 0x0001 << ( iX & 0x0003 );

    ASSERT( 0 <= iByte && iByte < m_nBytes );

    return 0 != ( m_pby[iByte] & iMask );
}

//--------------------------------------------------------------------------
// CBitMatrix::Set
//--------------------------------------------------------------------------
void CBitMatrix::Set( int iX, int iY )
{
    int iByte = m_iPitchBytes * iY + ( iX >> 3 );
    int iMask = 0x0001 << ( iX & 0x0003 );

    ASSERT( 0 <= iByte && iByte < m_nBytes );

    m_pby[iByte] |= iMask;
}

//--------------------------------------------------------------------------
// CBitMatrix::Reset
//--------------------------------------------------------------------------
void CBitMatrix::Reset( int iX, int iY )
{
    int iByte = m_iPitchBytes * iY + ( iX >> 3 );
    int iMask = 0x0001 << ( iX & 0x0003 );

    m_pby[iByte] &= ~iMask;
}

//--------------------------------------------------------------------------
// CBitMatrix::Clear
//--------------------------------------------------------------------------
void CBitMatrix::Clear( )
{
    memset( m_pby, 0, m_nBytes );
}

//----------------------- C H e x V a l i d M a t r i x --------------------

//--------------------------------------------------------------------------
// CHexValidMatrix
//--------------------------------------------------------------------------
CHexValidMatrix::CHexValidMatrix( int cxPowerOf2, int cyPowerOf2 )
    : CBitMatrix( 0x0001 << cxPowerOf2, 0x0001 << cyPowerOf2 ), m_iMaskX( ( 0x0001 << cxPowerOf2 ) - 1 ),
      m_iMaskY( ( 0x0001 << cyPowerOf2 ) - 1 )
{
}

//--------------------------------------------------------------------------
// Description of a few of the coordinate systems used here:
//
// Name		Type			Units			Desc
// ==========================================================================
// Window 	CPoint 			Pixel 	Coordinates relative to window's UL corner
// --------------------------------------------------------------------------
// View	  	CPoint 			Pixel		The coordinate system that windows are
//												viewports into
// --------------------------------------------------------------------------
//	HexView	CViewHexCoord	Hex	  	View hex coordinates - Dir independent
//				(a CPoint)					x increasing right, y increasing down.
//
//										  		 /\    /\	 /\ 
//										  		/	\  /	\  /  \ 
//										  	  /	 \/	 \/	 \ 
//							y = 0 ->		 /\0,0 /\1,0 /\2.0 /
//										  	/	\	/	\	/	\	/
//										  /	 \/	 \/	 \/
//							y = 1 ->	  \-1,1/\0,1 /\1,1 /\2,1
//										  	\ 	/	\  /	\  /  \ 
//										  	 \/	 \/	 \/	 \ 
//							y = 2	->	  	  \0,2 /\1,2 /\2,2 /
//										  	  	\	/	\	/	\	/
//										  	  	 \/	 \/	 \/
//
// --------------------------------------------------------------------------
// HexMap	CHexCoord		Hex			Map hex coords
// --------------------------------------------------------------------------
// Map		CMapLoc			Hex * 64		Flat map coords
//
//												 |
//												 | dir = 2
//												 v
//										-y\		  / x
//											\	/\	 /
//											 \/  \/			  Diagram of Map coord
//											 /    \			  system and the hex at the
//											/   	 \	 		  origin.
//									 --->	\    	 /	 <---
//								 dir = 1	 \    /	 dir = 3
//											 /\  /\ 
//										   /	\/	 \ 
//										 -x		  y
//												^
//												| dir = 0
//												|
//
// --------------------------------------------------------------------------
// World		CMapLoc3D	Hex * 64		3D Map coords (Map + height)
//								[0-127] height
//
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// CHitInfo::CHitInfo
//--------------------------------------------------------------------------
CHitInfo::CHitInfo( const CHexCoord& hexcoord )
    : m_punit( NULL ), m_pvehicle( NULL ), m_pbuilding( NULL ), m_pbridgeunit( NULL ), m_hexcoord( hexcoord )
{
    m_phex = theMap.GetHex( m_hexcoord );
}

//--------------------------------------------------------------------------
// CHitInfo::CHitInfo
//--------------------------------------------------------------------------
CHitInfo::CHitInfo( const CHitInfo& hitinfo )
    : m_punit( hitinfo.m_punit ), m_phex( hitinfo.m_phex ), m_pbridgeunit( hitinfo.m_pbridgeunit ),
      m_pvehicle( hitinfo.m_pvehicle ), m_pbuilding( hitinfo.m_pbuilding ), m_hexcoord( hitinfo.m_hexcoord )
{
}

//--------------------------------------------------------------------------
// CHitInfo::AssertValid
//--------------------------------------------------------------------------
void CHitInfo::AssertValid( )
{
    ASSERT_VALID_OR_NULL( m_punit );
    ASSERT_VALID_OR_NULL( m_phex );
    ASSERT_VALID_OR_NULL( m_pbridgeunit );
    ASSERT_VALID_OR_NULL( m_pvehicle );
    ASSERT_VALID_OR_NULL( m_pbuilding );

    ASSERT( m_punit || m_phex || m_pbridgeunit );
    ASSERT( !m_punit || m_pvehicle || m_pbuilding );
    ASSERT( !m_pvehicle || !( m_pbuilding || m_phex || m_pbridgeunit ) );
}

//--------------------------------------------------------------------------
// CHitInfo::SetVehicleHit
//--------------------------------------------------------------------------
void CHitInfo::SetVehicleHit( CVehicle* pvehicle )
{
    ASSERT_VALID( this );

    m_punit = m_pvehicle = pvehicle;
    m_phex               = NULL;

    ASSERT_VALID( this );
}

//--------------------------------------------------------------------------
// CHitInfo::SetBuildingHit
//--------------------------------------------------------------------------
void CHitInfo::SetBuildingHit( CBuilding* pbuilding )
{
    ASSERT_VALID( this );

    m_punit = m_pbuilding = pbuilding;
    m_pvehicle            = NULL;
    m_phex                = NULL;

    ASSERT_VALID( this );
}

//--------------------------------------------------------------------------
// CHitInfo::SetBridgeHit
//--------------------------------------------------------------------------
void CHitInfo::SetBridgeHit( CBridgeUnit* pbridgeunit )
{
    ASSERT_VALID( this );

    m_pbridgeunit = pbridgeunit;

    m_punit    = NULL;
    m_pvehicle = NULL;
    m_phex     = NULL;

    ASSERT_VALID( this );
}

//---------------------------- C H e x C o o r d C o l u m n I t e r ----------

class CHexCoordColumnIter
#ifdef _DEBUG
    : public CObject
#endif
{
  public:
    CHexCoordColumnIter( const CAnimAtr* panimatr, const CHexCoord& hexcoord, CPoint pt, BOOL bStopCondition = FALSE,
                         int nOverscanPixels = theStructures.GetTallestBuildingHeight( xiZoom ) );

    const CHexCoord* Value( ) { return m_bDone ? NULL : &m_hexcoord; }

    void ToNext( );
    void ToPrev( );

#ifdef _DEBUG
    void AssertValid( ) const;
#endif

  private:
    CHexCoord       m_hexcoord;
    const CAnimAtr* m_panimatr;
    CPoint          m_pt;
    int             m_iMaxTop;
    BOOL            m_bStopCondition;
    BOOL            m_bRight;
    BOOL            m_bOdd;
    BOOL            m_bDone;
};

#ifdef _DEBUG

void CHexCoordColumnIter::AssertValid( ) const
{
    ASSERT( m_panimatr );

    ASSERT( !m_bStopCondition || ( m_iMaxTop > 0 && m_iMaxTop < 5000 ) );
    ASSERT( TRUE == m_bRight || FALSE == m_bRight );
    ASSERT( TRUE == m_bDone || FALSE == m_bDone );
    ASSERT( TRUE == m_bOdd || FALSE == m_bOdd );
    ASSERT( TRUE == m_bStopCondition || FALSE == m_bStopCondition );
}

#endif

//---------------------------------------------------------------------------
// CHexCoordColumnIter::CHexCoordColumnIter
//---------------------------------------------------------------------------
CHexCoordColumnIter::CHexCoordColumnIter( const CAnimAtr*  panimatr,
                                          const CHexCoord& hexcoord,  // Start location
                                          CPoint           pt,        // Point in the hex, for debugging
                                          BOOL bStopCondition, int nOverscanPixels )
    : m_hexcoord( hexcoord ), m_panimatr( panimatr ), m_pt( pt ), m_bDone( FALSE ), m_bOdd( FALSE ), m_iMaxTop( 0 ),

      m_bStopCondition( bStopCondition )
{
    m_bRight = m_panimatr->IsPtRight( pt, hexcoord );

    if ( m_bStopCondition )
        m_iMaxTop = panimatr->m_dibwnd.GetWinSize( ).cy + nOverscanPixels;
}

//---------------------------------------------------------------------------
// CHexCoordColumnIter::ToNext
//---------------------------------------------------------------------------
void CHexCoordColumnIter::ToNext( )
{
    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT( !m_bDone );

    const signed char aaaacInc[4][2][2][2] =  // dir x odd x right x x/y
        { 0, 1, -1, 0, -1, 0, 0, 1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 1, 0, 1, 0, 0, -1, 1, 0, 0, 1, 0, 1, 1, 0 };

    m_bOdd = !m_bOdd;

    const signed char* pcInc = aaaacInc[m_panimatr->m_iDir][m_bOdd][m_bRight];

    m_hexcoord.X( ) += pcInc[0];
    m_hexcoord.Y( ) += pcInc[1];

#ifdef _DEBUG
    m_panimatr->CheckPt( m_pt, m_hexcoord );
#endif

    if ( m_bStopCondition )
    {
        CRect rect;

        if ( m_panimatr->CalcWindowHexBound( m_hexcoord, rect ) )
            if ( rect.top > m_iMaxTop )
                m_bDone = TRUE;
    }
}

//---------------------------------------------------------------------------
// CHexCoordColumnIter::ToPrev
//---------------------------------------------------------------------------
void CHexCoordColumnIter::ToPrev( )
{
    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT( !m_bDone );

    const signed char aaaacInc[4][2][2][2] =  // dir x odd x right x x/y
        { 1, 0, 0, -1, 0, -1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, -1, 0, 0, 1, 0, 1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1 };

    m_bOdd = !m_bOdd;

    const signed char* pcInc = aaaacInc[m_panimatr->m_iDir][m_bOdd][m_bRight];

    m_hexcoord.X( ) += pcInc[0];
    m_hexcoord.Y( ) += pcInc[1];
}

//---------------------------------------------------------------------------
// CViewHexCoord::CViewHexCoord	- Convert HexMap coords to HexView coords
//
// See CHexCoord::CHexCoord( CHexViewCoord ) for the inverse mapping
//---------------------------------------------------------------------------
CViewHexCoord::CViewHexCoord( const CHexCoord& hexcoord )
{
    switch ( xiDir )
    {
    case 0:
        x = hexcoord.X( ) + hexcoord.Y( );
        y = -hexcoord.X( ) + hexcoord.Y( );
        break;

    case 1:
        x = -hexcoord.X( ) + hexcoord.Y( );
        y = -hexcoord.X( ) - hexcoord.Y( );
        break;

    case 2:
        x = -hexcoord.X( ) - hexcoord.Y( );
        y = hexcoord.X( ) - hexcoord.Y( );
        break;

    case 3:
        x = hexcoord.X( ) - hexcoord.Y( );
        y = hexcoord.X( ) + hexcoord.Y( );
        break;
    }

    x >>= 1;
}

//---------------------------------------------------------------------------
// CHexCoord::CHexCoord	- Convert HexView coords to HexMap coords
//
// See CViewHexCoord::CViewHexCoord( CHexCoord ) for the inverse mapping
//---------------------------------------------------------------------------
CHexCoord::CHexCoord( const CViewHexCoord& viewhexcoord,
                      BOOL                 bRight )  // Jog right or left (from center hex)
{
    int iY1 = viewhexcoord.y + !bRight;
    int iY2 = viewhexcoord.y + bRight;

    iY1 >>= 1;
    iY2 >>= 1;

    int iX = viewhexcoord.x;

    switch ( xiDir )
    {
    case 0:
        m_iX = iX - iY1;
        m_iY = iX + iY2;
        break;

    case 1:
        m_iX = -iX - iY2;
        m_iY = iX - iY1;
        break;


    case 2:
        m_iX = -iX + iY1;
        m_iY = -iX - iY2;
        break;

    case 3:
        m_iX = iX + iY2;
        m_iY = -iX + iY1;
        break;
    }
}

//---------------------------------------------------------------------------
// CHexCoord::Flatten - Set 4 corners to same height and invalidate surrounding 3x3 hexes
//---------------------------------------------------------------------------
void CHexCoord::Flatten( int iAlt ) const
{
    theMap.GetHex( X( ), Y( ) )->SetAlt( iAlt );
    theMap.GetHex( X( ) + 1, Y( ) )->SetAlt( iAlt );
    theMap.GetHex( X( ) + 1, Y( ) + 1 )->SetAlt( iAlt );
    theMap.GetHex( X( ), Y( ) + 1 )->SetAlt( iAlt );

    for ( int iX = -1; iX <= 1; ++iX )
        for ( int iY = -1; iY <= 1; ++iY ) CHexCoord( X( ) + iX, Y( ) + iY ).SetInvalidated( );
}

//---------------------------------------------------------------------------
// CHexCoord::ToNearestHex
//---------------------------------------------------------------------------
CHexCoord& CHexCoord::ToNearestHex( const CHexCoord& hexcoord )
{
    int iWidth   = theMap.Get_eX( );
    int iHeight  = theMap.Get_eY( );
    int iWidth2  = iWidth >> 1;
    int iHeight2 = iHeight >> 1;

    while ( m_iX - hexcoord.X( ) > iWidth2 ) m_iX -= iWidth;

    while ( m_iX - hexcoord.X( ) < -iWidth2 ) m_iX += iWidth;

    while ( m_iY - hexcoord.Y( ) > iHeight2 ) m_iY -= iHeight;

    while ( m_iY - hexcoord.Y( ) < -iHeight2 ) m_iY += iHeight;

    return *this;
}

//---------------------------------------------------------------------------
// CAnimAtr::~CAnimAtr
//---------------------------------------------------------------------------
CAnimAtr::~CAnimAtr( )
{
}

//---------------------------------------------------------------------------
// CAnimAtr::Render
//---------------------------------------------------------------------------
void CAnimAtr::Render( )
{
    // Render each rect and add it to the list of rects to get blitted

    CDC* pdc = m_pwnd->GetDC( );

    try
    {
        for ( int i = 0; i < m_dirtyrects.m_nRectPaintCur; ++i )
            if ( m_dirtyrects.m_prectPaintCur[i].left != INT_MAX )
            {
                CRect rect = m_dirtyrects.m_prectPaintCur[i];

                if ( !pdc || pdc->RectVisible( &rect ) )
                {
                    m_dirtyrects.AddRect( &rect, CDirtyRects::RECT_LIST::LIST_BLT );

                    theMap.UpdateRect( *this, rect, CDrawParms::draw );
                }
            }
    }

    // and we move to try and fix it
    catch ( ... )
    {
        TRAP( );
        MoveCenterPixels( 1, 1 );
    }

    if ( pdc )
        m_pwnd->ReleaseDC( pdc );

    m_dirtyrects.UpdateLists( );  // Cur rect list <- Next rect list
}

//---------------------------------------------------------------------------
// CAnimAtr::WorldToCenterWorld
//---------------------------------------------------------------------------
CMapLoc3D CAnimAtr::WorldToCenterWorld( const CMapLoc3D& maploc3d ) const
{
    // Map the location to the one nearest the window center

    CMapLoc3D maploc3d2 = maploc3d;

    int iWidth  = theMap.Get_eX( ) << 6;
    int iWidth2 = iWidth >> 1;

    int iHeight  = theMap.Get_eY( ) << 6;
    int iHeight2 = iHeight >> 1;

    while ( maploc3d2.x - m_maploc.x > iWidth2 ) maploc3d2.x -= iWidth;

    while ( m_maploc.x - maploc3d2.x > iWidth2 ) maploc3d2.x += iWidth;

    while ( maploc3d2.y - m_maploc.y > iHeight2 ) maploc3d2.y -= iHeight;

    while ( m_maploc.y - maploc3d2.y > iHeight2 ) maploc3d2.y += iHeight;

    return maploc3d2;
}

//---------------------------------------------------------------------------
// CAnimAtr::ViewToWindow
//---------------------------------------------------------------------------
CPoint CAnimAtr::ViewToWindow( CPoint ptView ) const
{
    return ptView - m_ptUL;

    ASSERT_STRICT( m_iZoom >= theApp.GetZoomData( )->GetFirstZoom( ) && m_iZoom < NUM_ZOOM_LEVELS );

    CPoint pt  = ptView - m_ptUL;
    int    iW1 = theMap.Get_eX( ) << ( HEX_HT_PWR - m_iZoom + 1 );
    int    iH1 = theMap.Get_eY( ) << ( HEX_HT_PWR - m_iZoom );
    int    iW2 = iW1 >> 1;
    int    iH2 = iH1 >> 1;

    while ( pt.x >= iW1 ) pt.x -= iW2;

    while ( pt.x <= -iW1 ) pt.x += iW2;

    while ( pt.y >= iH1 ) pt.y -= iH2;

    while ( pt.y <= -iH1 ) pt.y += iH2;

    return pt;
}

//---------------------------------------------------------------------------
// CAnimAtr::Set - Specify attributes
//---------------------------------------------------------------------------
void CAnimAtr::Set( CMapLoc maplocCenter, int iDir, int iZoom )
{
    ASSERT_STRICT( iDir >= 0 && iDir < NUM_BLDG_DIRECTIONS );
    ASSERT_STRICT( iZoom >= theApp.GetZoomData( )->GetFirstZoom( ) && iZoom < NUM_ZOOM_LEVELS );

    m_iDir  = iDir;
    m_iZoom = iZoom;

    SetCenter( maplocCenter, SET_CENTER_NOP );

    ASSERT_STRICT_VALID_STRUCT( this );
}

//---------------------------------------------------------------------------
// CAnimAtr::SetCurrent - Set global variables for drawing in a window
//---------------------------------------------------------------------------
void CAnimAtr::SetCurrent( ) const
{
    xiDir     = m_iDir;
    xiZoom    = m_iZoom;
    xpdibwnd  = (CDIBWnd*)&m_dibwnd;
    xpanimatr = this;
}

//--------------------------------------------------------------------------
// CAnimAtr::Turn - Rotate the view
//--------------------------------------------------------------------------
int CAnimAtr::Turn( CAnimAtr::TURN_DIR eDir )
{
    ASSERT( m_iDir >= 0 && m_iDir < NUM_BLDG_DIRECTIONS );

    switch ( eDir )
    {
    case TURN_CLOCKWISE:

        --m_iDir;

        if ( m_iDir < 0 )
            m_iDir = NUM_BLDG_DIRECTIONS - 1;

        break;

    case TURN_COUNTERCLOCKWISE:

        ++m_iDir;

        if ( m_iDir >= NUM_BLDG_DIRECTIONS )
            m_iDir = 0;

        break;

    default:
        ASSERT( 0 );
    }

    SetCenter( m_maploc, SET_CENTER_INVALIDATE );  // Recalc UL corner view coord

    return m_iDir;
}

//--------------------------------------------------------------------------
// CAnimAtr::Zoom - Zoom in or out
//--------------------------------------------------------------------------
int  // New zoom level
CAnimAtr::Zoom( CAnimAtr::ZOOM_LEVEL eZoom )
{
    ASSERT( m_iZoom >= theApp.GetZoomData( )->GetFirstZoom( ) && m_iZoom < NUM_ZOOM_LEVELS );

    switch ( eZoom )
    {
    case ZOOM_IN:

        ASSERT( m_iZoom > theApp.GetZoomData( )->GetFirstZoom( ) );

        --m_iZoom;

        break;

    case ZOOM_OUT:

        ASSERT( m_iZoom < NUM_ZOOM_LEVELS - 1 );

        ++m_iZoom;

        break;
    }

    SetCenter( m_maploc, SET_CENTER_INVALIDATE );  // Recalc UL corner view coords

    return m_iZoom;
}

//--------------------------------------------------------------------------
// CAnimAtr::Resized - Call when the window's size changes
//--------------------------------------------------------------------------
void CAnimAtr::Resized( )
{
    // Recalc view coords of UL corner

    SetCenter( m_maploc, SET_CENTER_INVALIDATE );
}

//--------------------------------------------------------------------------
// CAnimAtr::SetCenter - Specify map coords corresponding to window center
//--------------------------------------------------------------------------
const CMapLoc& CAnimAtr::SetCenter( CMapLoc maploc, SET_CENTER_METHOD eMethod )
{
    if ( SET_CENTER_SCROLL == eMethod )
        Render( );

    m_maploc = maploc.Wrap( );

    // Interpolate height at center and map to view coords

    CMapLoc3D maploc3D( m_maploc );
    CPoint    ptCenter = WorldToView( maploc3D );
    CSize     size     = m_dibwnd.GetWinSize( );

    // Calc view coords of UL corner

    CPoint ptOldUL = m_ptUL;

    m_ptUL.x = ptCenter.x - ( size.cx >> 1 );
    m_ptUL.y = ptCenter.y - ( size.cy >> 1 );

    switch ( eMethod )
    {
    case SET_CENTER_SCROLL: {
        CPoint ptScroll = ptOldUL - m_ptUL;

        Scroll( ptScroll.x, ptScroll.y );

        break;
    }

    case SET_CENTER_INVALIDATE:

        GetDirtyRects( )->AddRect( NULL, CDirtyRects::RECT_LIST::LIST_PAINT_CUR );

        break;

    case SET_CENTER_NOP:

        break;
    }

    return m_maploc;
}

//--------------------------------------------------------------------------
// CAnimAtr::MoveCenterPixels - Move center by specified pixel offsets
//--------------------------------------------------------------------------
const CMapLoc& CAnimAtr::MoveCenterPixels( int iDelx, int iDely )
{
    CMapLoc maplocDelta = ViewToMap( CPoint( iDelx, iDely ), 0 );

    SetCenter( CMapLoc( m_maploc.x + maplocDelta.x, m_maploc.y + maplocDelta.y ), SET_CENTER_SCROLL );

    return m_maploc;
}

//--------------------------------------------------------------------------
// CAnimAtr::MoveCenterViewHexes - Move center by specified view hexes
//--------------------------------------------------------------------------
const CMapLoc&  // The new center
CAnimAtr::MoveCenterViewHexes( int iDelX, int iDelY )
{
    return MoveCenterPixels( iDelX * CGameMap::HexWid( m_iZoom ), iDelY * CGameMap::HexHt( m_iZoom ) );
}

//--------------------------------------------------------------------------
// CAnimAtr::MoveCenterHexes - Move center by specified map hexes
//--------------------------------------------------------------------------
const CMapLoc&  // The new center
CAnimAtr::MoveCenterHexes( int iDelX, int iDelY )
{
    CMapLoc maploc = m_maploc;

    iDelX *= MAX_HEX_HT;
    iDelY *= MAX_HEX_HT;

    switch ( m_iDir )
    {
    case 0:
        maploc.x += iDelX - iDelY;
        maploc.y += iDelX + iDelY;
        break;

    case 1:
        maploc.x += -iDelX - iDelY;
        maploc.y += iDelX - iDelY;
        break;

    case 2:
        maploc.x += -iDelX + iDelY;
        maploc.y += -iDelX - iDelY;
        break;

    case 3:
        maploc.x += iDelX + iDelY;
        maploc.y += -iDelX + iDelY;
        break;
    }

    return SetCenter( maploc, SET_CENTER_SCROLL );
}

//---------------------------------------------------------------------------
// CAnimAtr::Scroll - Specify attributes
//---------------------------------------------------------------------------
void CAnimAtr::Scroll( int iDelX, int iDelY )
{
    CRect rectDst = m_dibwnd.GetDIB( )->Scroll( iDelX, iDelY );

    if ( !rectDst.IsRectEmpty( ) )
    {
        // Just blit the new scrolled region

        GetDirtyRects( )->AddRect( &rectDst, CDirtyRects::RECT_LIST::LIST_BLT );

        CDIB* pdib = m_dibwnd.GetDIB( );

        ASSERT_VALID( pdib );

        int iLeft, iRight, iTop, iBottom;
        int iHeight, iWidth;

        iHeight = pdib->GetHeight( );
        iWidth  = pdib->GetWidth( );

        // Invalidate the remaining vertical rect to force its re-render

        if ( iDelX > 0 )
        {
            iLeft  = 0;
            iRight = iDelX;
        }
        else
        {
            iLeft  = iWidth + iDelX;
            iRight = iWidth;
        }

        iTop    = 0;
        iBottom = iHeight;

        CRect rect;

        rect.SetRect( iLeft, iTop, iRight, iBottom );

        if ( !rect.IsRectEmpty( ) )
            GetDirtyRects( )->AddRect( &rect, CDirtyRects::RECT_LIST::LIST_PAINT_CUR );

        // Invalidate the remaining horizontal rect to force its re-render

        if ( iDelX > 0 )
        {
            iLeft  = iDelX;
            iRight = iWidth;
        }
        else
        {
            iLeft  = 0;
            iRight = iWidth + iDelX;
        }

        if ( iDelY > 0 )
        {
            iTop    = 0;
            iBottom = iDelY;
        }
        else
        {
            iTop    = iHeight + iDelY;
            iBottom = iHeight;
        }

        rect.SetRect( iLeft, iTop, iRight, iBottom );

        if ( !rect.IsRectEmpty( ) )
            GetDirtyRects( )->AddRect( &rect, CDirtyRects::RECT_LIST::LIST_PAINT_CUR );
    }
    else
        GetDirtyRects( )->AddRect( NULL, CDirtyRects::RECT_LIST::LIST_PAINT_CUR );
}

//--------------------------------------------------------------------------
// CAnimAtr::MapToWindowHex - Get Window coords of 4 corners of a hex
//--------------------------------------------------------------------------
BOOL                                          // Is the hex facing the viewer?
CAnimAtr::MapToWindowHex( const CHexCoord& hexcoord,
                          CPoint           aptHex[4] )  // Output in 2D window coords (L,T,R,B)
    const
{
    CMapLoc3D amaploc3d[4];

    hexcoord.GetWorldHex( amaploc3d );

    return WorldToWindowHex( amaploc3d, aptHex );
}

//--------------------------------------------------------------------------
// CAnimAtr::WorldToWindowHex - Get Window coords of 4 corners of a hex
//--------------------------------------------------------------------------
BOOL                                                 // Is the hex facing the viewer?
CAnimAtr::WorldToWindowHex( CMapLoc3D amaploc3d[4],  // Input hex corner world coords
                            CPoint    aptHex[4] )       // Output in 2D window coords (L,T,R,B)
    const
{
    static int xaaiIndex[4][4] = { { 0, 1, 2, 3 }, { 3, 0, 1, 2 }, { 2, 3, 0, 1 }, { 1, 2, 3, 0 } };

    int* pi = xaaiIndex[m_iDir];

    for ( int i = 0; i < 4; ++i ) aptHex[*pi++] = WorldToWindow( amaploc3d[i] );

    return aptHex[1].y < aptHex[3].y;
}

//--------------------------------------------------------------------------
// CAnimAtr::CalcWindowHexBoundClipped -
//--------------------------------------------------------------------------
BOOL                                                // TRUE if rect is non-0
CAnimAtr::CalcWindowHexBoundClipped( const CHexCoord& hexcoord,
                                     CRect&           rect )  // output: Window coords of hex's bounding rect
    const                                           // clipped to the area window
{
    if ( !CalcWindowHexBound( hexcoord, rect ) )
        return FALSE;

    CSize sizeWin = m_dibwnd.GetWinSize( );
    CRect rectWin = CRect( CPoint( 0, 0 ), sizeWin );

    rect &= rectWin;

    return rect.IsRectNull( );
}

//--------------------------------------------------------------------------
// CAnimAtr::CalcWindowHexBound
//--------------------------------------------------------------------------
BOOL                                         // TRUE if rect is non-zero
CAnimAtr::CalcWindowHexBound( const CHexCoord& hexcoord,
                              CRect&           rect )  // output: Window coords of hex's bounding rect
    const
{
    CPoint aptHex[4];

    if ( !MapToWindowHex( hexcoord, aptHex ) )
        return FALSE;

    int iMinYIndex = 0;
    int iMaxYIndex = 0;
    int iMinY      = aptHex[0].y;
    int iMaxY      = aptHex[0].y;

    for ( int i = 1; i < 4; ++i )
        if ( aptHex[i].y < iMinY )
        {
            iMinY      = aptHex[i].y;
            iMinYIndex = i;
        }
        else if ( aptHex[i].y > iMaxY )
        {
            iMaxY      = aptHex[i].y;
            iMaxYIndex = i;
        }

    rect = CRect( aptHex[0].x, aptHex[iMinYIndex].y, aptHex[2].x, aptHex[iMaxYIndex].y );

    return TRUE;
}

//--------------------------------------------------------------------------
// CAnimAtr::IsHexHit - Is the cursor in the hex?
//--------------------------------------------------------------------------
BOOL CAnimAtr::IsHexHit( const CHexCoord& hexcoord, CPoint ptCursor, CRect& rrectBound ) const
{
    CPoint aptHex[4];

    // Get window coords of hex corners. If hex not facing viewer, not a hit

    BOOL bVisible = MapToWindowHex( hexcoord, aptHex );

    rrectBound = CRect( aptHex[0].x, min( aptHex[0].y, min( aptHex[1].y, aptHex[2].y ) ), aptHex[2].x,
                        max( aptHex[0].y, max( aptHex[2].y, aptHex[3].y ) ) );

    if ( !bVisible )
        return FALSE;

    if ( !rrectBound.PtInRect( ptCursor ) )
        return FALSE;

    // Find hex's min/max y for the cursor's x

    int fixStartTopY;
    int fixStartBotY;
    int fixTopDY;
    int fixBotDY;
    int fixDX;
    int iShiftW = HEX_HT_PWR - xiZoom;

    if ( ptCursor.x < aptHex[1].x )
    {
        fixStartTopY = ( aptHex[0].y << 16 ) + int( 0x00008000 );
        fixStartBotY = fixStartTopY;
        fixTopDY     = ( aptHex[1].y - aptHex[0].y ) << ( 16 - iShiftW );
        fixBotDY     = ( aptHex[3].y - aptHex[0].y ) << ( 16 - iShiftW );
        fixDX        = ( ptCursor.x - aptHex[0].x ) << 16;
    }
    else
    {
        fixStartTopY = ( aptHex[1].y << 16 ) + int( 0x00008000 );
        fixStartBotY = ( aptHex[3].y << 16 ) + int( 0x00008000 );
        fixTopDY     = ( aptHex[2].y - aptHex[1].y ) << ( 16 - iShiftW );
        fixBotDY     = ( aptHex[2].y - aptHex[3].y ) << ( 16 - iShiftW );
        fixDX        = ( ptCursor.x - aptHex[1].x ) << 16;
    }

    int fixTopY = fixDX;
    int fixBotY = fixDX;

    FIXMUL( fixTopY, fixTopDY );
    FIXMUL( fixBotY, fixBotDY );

    fixTopY += fixStartTopY;
    fixBotY += fixStartBotY;

    // Check if the cursor's y is within the min/max y
    // (we use top/left fill convention)

    return ptCursor.y >= ( fixTopY >> 16 ) && ptCursor.y < ( fixBotY >> 16 );
}

//--------------------------------------------------------------------------
// CAnimAtr::CheckPt -
//--------------------------------------------------------------------------
#ifdef _DEBUG
void CAnimAtr::CheckPt( CPoint,  //			  pt,
                        const CHexCoord& hexcoord ) const
{
    CPoint apthex[4];

    MapToWindowHex( hexcoord, apthex );

    ASSERT_STRICT( pt.x >= apthex[0].x );
    ASSERT_STRICT( pt.x <= apthex[2].x );
}
#endif

//--------------------------------------------------------------------------
// CAnimAtr::IsPtRight - Is the point in the right half of its hex?
//--------------------------------------------------------------------------
BOOL CAnimAtr::IsPtRight( CPoint           pt,         // Cursor location in window coords
                          const CHexCoord& hexcoord )  // hex containing the point
    const
{
    CPoint apthex[4];

    MapToWindowHex( hexcoord, apthex );

    ASSERT_STRICT( pt.x >= apthex[0].x );
    ASSERT_STRICT( pt.x <= apthex[2].x );

    return pt.x >= apthex[1].x;
}

//------------------------ C T i l e D r a w I n f o -----------------------
//
// Draws a tile, and defines operator < for display list position comparison

class CTileDrawInfo
{

  public:
    enum TILE_TYPE
    {
        vehicle,
        building,
        projectile,
        bridge,
        explosion,
        tree
    };

    void Init( CUnitTile* ptile, TILE_TYPE, const CHexCoord& hexcoord, const CMapLoc& );

    virtual BOOL operator<( const CTileDrawInfo& tiledrawinfo ) const;
    virtual BOOL operator==( const CTileDrawInfo& tiledrawinfo ) const;

    CUnitTile* GetTile( ) const
    {
        ASSERT( m_punittile );
        return m_punittile;
    }
    const CHexCoord& GetHexCoord( ) const { return m_hexcoord; }
    const CMapLoc&   GetMapLoc( ) const { return m_maploc; }

    virtual void Draw( ) { m_punittile->Draw( m_hexcoord ); }

    TILE_TYPE GetTileType( ) const { return m_etiletype; }

  private:
    CUnitTile* m_punittile;
    CHexCoord  m_hexcoord;
    CPoint     m_ptCenter;
    CMapLoc    m_maploc;
    TILE_TYPE  m_etiletype;
};

//--------------------------------------------------------------------------
// CTileDrawInfo::operator ==
//--------------------------------------------------------------------------
BOOL CTileDrawInfo::operator==( const CTileDrawInfo& tiledrawinfo ) const
{
    return m_punittile == tiledrawinfo.m_punittile && m_maploc == tiledrawinfo.m_maploc;
}

//--------------------------------------------------------------------------
// CTileDrawInfo::Init
//--------------------------------------------------------------------------
void CTileDrawInfo::Init( CUnitTile* punittile, CTileDrawInfo::TILE_TYPE etiletype, const CHexCoord& hexcoord,
                          const CMapLoc& maploc )
{
    ASSERT_STRICT_VALID( punittile );

    m_punittile = punittile;
    m_etiletype = etiletype;
    m_hexcoord  = hexcoord;
    m_maploc    = maploc;

    CMapLoc3D maploc3d( maploc.x, maploc.y, 0 );
    CMapLoc3D maploc3d2( xpanimatr->WorldToCenterWorld( maploc3d ) );

    switch ( xiDir )
    {
    case 0:
        m_ptCenter.x = maploc3d2.y;
        m_ptCenter.y = maploc3d2.y - maploc3d2.x;
        break;

    case 1:
        m_ptCenter.x = maploc3d2.y;
        m_ptCenter.y = -maploc3d2.y - maploc3d2.x;
        break;

    case 2:
        m_ptCenter.x = -maploc3d2.y;
        m_ptCenter.y = -maploc3d2.y + maploc3d2.x;
        break;

    case 3:
        m_ptCenter.x = -maploc3d2.y;
        m_ptCenter.y = maploc3d2.y + maploc3d2.x;
        break;
    }
}

//--------------------------------------------------------------------------
// CTileDrawInfo::operator <	- Is this tile behind the other one?
//--------------------------------------------------------------------------
BOOL CTileDrawInfo::operator<( const CTileDrawInfo& tiledrawinfo ) const
{
    // Make sure farthest objects drawn first

    if ( m_ptCenter.y != tiledrawinfo.m_ptCenter.y )
        return m_ptCenter.y < tiledrawinfo.m_ptCenter.y;

    // If same distance, sort on x

    if ( m_ptCenter.x != tiledrawinfo.m_ptCenter.x )
        return m_ptCenter.x < tiledrawinfo.m_ptCenter.x;

    // If same location, treat as duplicate if the same unit

    return m_punittile != tiledrawinfo.m_punittile;
}

//--------------------- C E x p l o s i o n D r a w I n f o --------------------
//

class CExplosionDrawInfo : public CTileDrawInfo
{

  public:
    void Init( CProjBase* ptile, const CHexCoord& hexcoord );

    virtual BOOL operator<( const CTileDrawInfo& tiledrawinfo ) const;
};

//--------------------------------------------------------------------------
// CExplosionDrawInfo::Init
//--------------------------------------------------------------------------
void CExplosionDrawInfo::Init( CProjBase* pprojbase, const CHexCoord& hexcoord )
{
    CTileDrawInfo::Init( pprojbase, explosion, hexcoord, pprojbase->GetMapLoc( ) );
}

//--------------------------------------------------------------------------
// CExplosionDrawInfo::operator <	- Is this tile behind the other one?
//--------------------------------------------------------------------------
BOOL CExplosionDrawInfo::operator<( const CTileDrawInfo& tiledrawinfo ) const
{
    switch ( tiledrawinfo.GetTileType( ) )
    {
    case CTileDrawInfo::vehicle:
    case CTileDrawInfo::building:

        return !tiledrawinfo.operator<( *this );
    }

    return CTileDrawInfo::operator<( tiledrawinfo );
}

//-------------------- C S t r u c t u r e D r a w I n f o ------------------
//
// CTileDrawInfo subclass for 1- and 2-piece structures

class CStructureDrawInfo : public CTileDrawInfo
{

  public:
    void Init( CUnitTile* punittile, TILE_TYPE eType, const CHexCoord& hexcoord, const CMapLoc& maploc,
               CStructureSprite::LAYER_TYPE eLayer = CStructureSprite::FOREGROUND_LAYER );

    BOOL operator<( const CTileDrawInfo& tiledrawinfo ) const;
    BOOL operator==( const CTileDrawInfo& tiledrawinfo ) const;

    void Draw( ) { GetTile( )->DrawLayer( GetHexCoord( ), m_eLayer ); }

    CUnitTile* GetTile( ) const { return (CUnitTile*)CTileDrawInfo::GetTile( ); }

    CStructureSprite::LAYER_TYPE GetLayer( ) const { return m_eLayer; }

  private:
    CStructureSprite::LAYER_TYPE m_eLayer;
};

//--------------------------------------------------------------------------
// CStructureDrawInfo::Init
//--------------------------------------------------------------------------
void CStructureDrawInfo::Init( CUnitTile* punittile, CTileDrawInfo::TILE_TYPE eType, const CHexCoord& hexcoord,
                               const CMapLoc&               maploc,  // For drawing order
                               CStructureSprite::LAYER_TYPE eLayer )
{
    CTileDrawInfo::Init( punittile, eType, hexcoord, maploc );

    m_eLayer = eLayer;

    ASSERT( building == eType || bridge == eType || tree == eType );
}

//--------------------------------------------------------------------------
// CStructureDrawInfo::operator ==
//--------------------------------------------------------------------------
BOOL CStructureDrawInfo::operator==( const CTileDrawInfo& tiledrawinfo ) const
{
    BOOL bSame = CTileDrawInfo::operator==( tiledrawinfo );

    if ( bSame )
        return ( (CStructureDrawInfo&)tiledrawinfo ).m_eLayer == m_eLayer;

    return FALSE;
}

//--------------------------------------------------------------------------
// CStructureDrawInfo::operator <
//--------------------------------------------------------------------------
BOOL CStructureDrawInfo::operator<( const CTileDrawInfo& tiledrawinfo ) const
{
    switch ( tiledrawinfo.GetTileType( ) )
    {
    case vehicle:

        // CVehicleDrawInfo code handles bridges and buildings
        return !tiledrawinfo.operator<( *this );

    case explosion:

#ifdef _GG
        //			GGTRAP();
#endif

        // If this is a building and the explosion is inside the building footprint column, return TRUE,

        if ( building == GetTileType( ) )
        {
            CBuilding* pbuilding = (CBuilding*)GetTile( );
            CProjBase* pprojbase = (CProjBase*)tiledrawinfo.GetTile( );

            ASSERT( pbuilding );
            ASSERT( pprojbase );

            CMapLoc maplocBuilding  = pbuilding->GetMapLoc( );
            CMapLoc maplocExplosion = pprojbase->GetMapLoc( );

            maplocExplosion.ToNearest( maplocBuilding );

            CRect rectBuilding = CRect( maplocBuilding.x - ( pbuilding->GetCX( ) << HEX_HT_PWR - 1 ),
                                        maplocBuilding.y - ( pbuilding->GetCY( ) << HEX_HT_PWR - 1 ),
                                        maplocBuilding.x + ( pbuilding->GetCX( ) << HEX_HT_PWR - 1 ),
                                        maplocBuilding.y + ( pbuilding->GetCY( ) << HEX_HT_PWR - 1 ) );

            CPoint ptExplosion = maplocExplosion;

            BOOL bExplosionInside = rectBuilding.PtInRect( ptExplosion );

            if ( bExplosionInside )
                return TRUE;
        }

        break;
    }

    // If same tile and same location,
    // return TRUE if comparing background layer against foreground layer,
    // otherwise FALSE

    if ( GetTile( ) == tiledrawinfo.GetTile( ) && GetMapLoc( ) == tiledrawinfo.GetMapLoc( ) )
    {
        CStructureDrawInfo* ptiledrawinfo = (CStructureDrawInfo*)&tiledrawinfo;

        if ( CStructureSprite::BACKGROUND_LAYER == m_eLayer &&
             CStructureSprite::FOREGROUND_LAYER == ptiledrawinfo->m_eLayer )
            return TRUE;
        else
            return FALSE;
    }

    // Otherwise use default comparison

    else
        return CTileDrawInfo::operator<( tiledrawinfo );
}

//---------------------- C V e h i c l e D r a w I n f o --------------------
//

class CVehicleDrawInfo : public CTileDrawInfo
{

  public:
    void Init( CVehicle* pvehicle, const CHexCoord& hexcoord );

    virtual BOOL operator<( const CTileDrawInfo& tiledrawinfo ) const;
};

//--------------------------------------------------------------------------
// CVehicleDrawInfo::Init
//--------------------------------------------------------------------------
void CVehicleDrawInfo::Init( CVehicle* pvehicle, const CHexCoord& hexcoord )
{
    CTileDrawInfo::Init( pvehicle, vehicle, hexcoord, pvehicle->GetMapLoc( ) );
}

//--------------------------------------------------------------------------
// CVehicleDrawInfo::operator <
//--------------------------------------------------------------------------
BOOL CVehicleDrawInfo::operator<( const CTileDrawInfo& tiledrawinfo ) const
{
    switch ( tiledrawinfo.GetTileType( ) )
    {
    case bridge:

        // If the vehicle is under the bridge, return TRUE.
        // If it's on the bridge,
        // 	return FALSE if bridge back layer,
        // 	return TRUE if bridge front layer.
        // Otherwise, do default comparison
        {
            CVehicle*    pvehicle = (CVehicle*)GetTile( );
            CBridgeUnit* pbridge  = (CBridgeUnit*)tiledrawinfo.GetTile( );

            CHexCoord hexcoordVehicle = GetMapLoc( );
            CHexCoord hexcoordBridge  = pbridge->GetHex( );

            if ( hexcoordVehicle == hexcoordBridge )
            {
                CVehicle* pvehicle = (CVehicle*)GetTile( );

                if ( !pvehicle->IsOnWater( ) )
                    return CStructureSprite::FOREGROUND_LAYER == ( (CStructureDrawInfo*)&tiledrawinfo )->GetLayer( );
                else
                    return TRUE;
            }
        }

        break;

    case explosion:

        // If the explosion is near the vehicle, return TRUE,
        // Otherwise do default comparison
        {
            CVehicle*  pvehicle  = (CVehicle*)GetTile( );
            CProjBase* pprojbase = (CProjBase*)tiledrawinfo.GetTile( );

            ASSERT( pvehicle );
            ASSERT( pprojbase );

            CMapLoc maplocVehicle   = pvehicle->GetMapLoc( );
            CMapLoc maplocExplosion = pprojbase->GetMapLoc( );

            BOOL bExplosionNear =
                32 > max( abs( maplocVehicle.x - maplocExplosion.x ), abs( maplocVehicle.y - maplocExplosion.y ) );

            if ( bExplosionNear )
                return TRUE;
        }

        break;

    case building:

        // If any part of the vehicle is inside,
        //		return FALSE if building back layer
        //		return TRUE if building front layer
        {
            CVehicle*  pvehicle         = (CVehicle*)GetTile( );
            CBuilding* pbuilding        = (CBuilding*)tiledrawinfo.GetTile( );
            CSubHex    subhexHead       = pvehicle->GetPtHead( );
            CHexCoord  hexcoordHead     = subhexHead;
            CHexCoord  hexcoordBuilding = pbuilding->GetHex( );

            BOOL bInside = hexcoordHead.X( ) >= hexcoordBuilding.X( ) &&
                           hexcoordHead.X( ) < hexcoordBuilding.X( ) + pbuilding->GetCX( ) &&
                           hexcoordHead.Y( ) >= hexcoordBuilding.Y( ) &&
                           hexcoordHead.Y( ) < hexcoordBuilding.Y( ) + pbuilding->GetCY( );

            if ( !bInside )  // Check tail hex
            {
                CSubHex   subhexTail   = pvehicle->GetPtTail( );
                CHexCoord hexcoordTail = subhexTail;

                bInside = hexcoordTail.X( ) >= hexcoordBuilding.X( ) &&
                          hexcoordTail.X( ) < hexcoordBuilding.X( ) + pbuilding->GetCX( ) &&
                          hexcoordTail.Y( ) >= hexcoordBuilding.Y( ) &&
                          hexcoordTail.Y( ) < hexcoordBuilding.Y( ) + pbuilding->GetCY( );
            }

            if ( bInside )
                return CStructureSprite::FOREGROUND_LAYER == ( (CStructureDrawInfo*)&tiledrawinfo )->GetLayer( );
        }

        break;
    }

    return CTileDrawInfo::operator<( tiledrawinfo );
}

//-------------------------- C D r a w I n f o P o o l ---------------------

class CDrawInfoPool
{

  public:
    CDrawInfoPool( ) {}
    ~CDrawInfoPool( ) { Reset( ); }

    CTileDrawInfo*      GetTileDrawInfo( CUnitTile* ptile, CTileDrawInfo::TILE_TYPE, const CHexCoord& hexcoord,
                                         const CMapLoc& );
    CExplosionDrawInfo* GetExplosionDrawInfo( CProjBase* ptile, const CHexCoord& hexcoord );
    CVehicleDrawInfo*   GetVehicleDrawInfo( CVehicle* pvehicle, const CHexCoord& hexcoord );
    CStructureDrawInfo* GetStructureDrawInfo(
        CUnitTile* punittile, CTileDrawInfo::TILE_TYPE eType, const CHexCoord& hexcoord, const CMapLoc& maploc,
        CStructureSprite::LAYER_TYPE eLayer = CStructureSprite::FOREGROUND_LAYER );

  private:
    static CTileDrawInfo      x_atiledrawinfo[];
    static CExplosionDrawInfo x_aexplosiondrawinfo[];
    static CVehicleDrawInfo   x_avehicledrawinfo[];
    static CStructureDrawInfo x_astructuredrawinfo[];

    static void Reset( )
    {
        x_nTileDrawInfo      = 0;
        x_nExplosionDrawInfo = 0;
        x_nVehicleDrawInfo   = 0;
        x_nStructureDrawInfo = 0;
    };

    static int x_nTileDrawInfo;
    static int x_nExplosionDrawInfo;
    static int x_nVehicleDrawInfo;
    static int x_nStructureDrawInfo;
};

const int MAX_TILE_INFO      = 500;
const int MAX_EXPLOSION_INFO = 500;
const int MAX_VEHICLE_INFO   = 500;
const int MAX_STRUCTURE_INFO = 500;

CTileDrawInfo      CDrawInfoPool::x_atiledrawinfo[MAX_TILE_INFO];
CExplosionDrawInfo CDrawInfoPool::x_aexplosiondrawinfo[MAX_EXPLOSION_INFO];
CVehicleDrawInfo   CDrawInfoPool::x_avehicledrawinfo[MAX_VEHICLE_INFO];
CStructureDrawInfo CDrawInfoPool::x_astructuredrawinfo[MAX_STRUCTURE_INFO];

int CDrawInfoPool::x_nTileDrawInfo;
int CDrawInfoPool::x_nExplosionDrawInfo;
int CDrawInfoPool::x_nVehicleDrawInfo;
int CDrawInfoPool::x_nStructureDrawInfo;

//--------------------------------------------------------------------------
// CDrawInfoPool::GetTileDrawInfo
//--------------------------------------------------------------------------
CTileDrawInfo* CDrawInfoPool::GetTileDrawInfo( CUnitTile* ptile, CTileDrawInfo::TILE_TYPE eType,
                                               const CHexCoord& hexcoord, const CMapLoc& maploc )
{
    CTileDrawInfo* pinfo = x_atiledrawinfo + x_nTileDrawInfo;

    pinfo->Init( ptile, eType, hexcoord, maploc );

    x_nTileDrawInfo = ( x_nTileDrawInfo + 1 ) % MAX_TILE_INFO;

    return pinfo;
}

//--------------------------------------------------------------------------
// CDrawInfoPool::GetExplosionDrawInfo
//--------------------------------------------------------------------------
CExplosionDrawInfo* CDrawInfoPool::GetExplosionDrawInfo( CProjBase* ptile, CHexCoord const& hexcoord )
{
    CExplosionDrawInfo* pinfo = x_aexplosiondrawinfo + x_nExplosionDrawInfo;

    pinfo->Init( ptile, hexcoord );

    x_nExplosionDrawInfo = ( x_nExplosionDrawInfo + 1 ) % MAX_EXPLOSION_INFO;

    return pinfo;
}

//--------------------------------------------------------------------------
// CDrawInfoPool::GetVehicleDrawInfo
//--------------------------------------------------------------------------
CVehicleDrawInfo* CDrawInfoPool::GetVehicleDrawInfo( CVehicle* pvehicle, const CHexCoord& hexcoord )
{
    CVehicleDrawInfo* pinfo = x_avehicledrawinfo + x_nVehicleDrawInfo;

    pinfo->Init( pvehicle, hexcoord );

    x_nVehicleDrawInfo = ( x_nVehicleDrawInfo + 1 ) % MAX_VEHICLE_INFO;

    return pinfo;
}

//--------------------------------------------------------------------------
// CDrawInfoPool::GetStructureDrawInfo
//--------------------------------------------------------------------------
CStructureDrawInfo* CDrawInfoPool::GetStructureDrawInfo( CUnitTile* punittile, CTileDrawInfo::TILE_TYPE eType,
                                                         const CHexCoord& hexcoord, const CMapLoc& maploc,
                                                         CStructureSprite::LAYER_TYPE eLayer )
{
    CStructureDrawInfo* pinfo = x_astructuredrawinfo + x_nStructureDrawInfo;

    pinfo->Init( punittile, eType, hexcoord, maploc, eLayer );

    x_nStructureDrawInfo = ( x_nStructureDrawInfo + 1 ) % MAX_STRUCTURE_INFO;

    return pinfo;
}

//--------------------------------------------------------------------------
// CAnimAtr::GetHit - Get building, bridge, vehicle, or hex under the cursor
//--------------------------------------------------------------------------
CHitInfo CAnimAtr::GetHit( CPoint pt )  // Cursor location in window coords
    const
{
    SetCurrent( );  // Set globals for sprite hit-testing

    CDrawParms::SetClipRect( m_dibwnd.GetRect( ) );

    // Get the hex closest to the viewer that the cursor intersects

    CHexCoord hexcoord = _WindowToHex( pt );

    // The hex gets returned if no unit intersections found

    CHex* phex = theMap.GetHex( hexcoord );

    ASSERT_STRICT( phex );

    CHitInfo hitinfo( hexcoord );

    // A heap - put everything we hit in here then pull out the one closest to viewr
    BTree<CTileDrawInfo> btreetiledrawinfo( FALSE );  // non-owning

    // Walk the column down to check for a building or vehicle hit

    CHexCoordColumnIter hexiter( this, hexcoord, pt, TRUE );

    if ( !hexiter.Value( ) )
        return hitinfo;

    int iDelX = MAX_HEX_HT << 1;

    CHexCoord hexcoordLeft  = *hexiter.Value( );
    CHexCoord hexcoordRight = *hexiter.Value( );
    CPoint    ptLeft( pt.x - iDelX, pt.y );
    CPoint    ptRight( pt.x + iDelX, pt.y );

    switch ( m_iDir )
    {
    case 0:
        --hexcoordLeft.X( );
        --hexcoordLeft.Y( );
        ++hexcoordRight.X( );
        ++hexcoordRight.Y( );
        break;

    case 1:
        ++hexcoordLeft.X( );
        --hexcoordLeft.Y( );
        --hexcoordRight.X( );
        ++hexcoordRight.Y( );
        break;

    case 2:
        ++hexcoordLeft.X( );
        ++hexcoordLeft.Y( );
        --hexcoordRight.X( );
        --hexcoordRight.Y( );
        break;

    case 3:
        --hexcoordLeft.X( );
        ++hexcoordLeft.Y( );
        ++hexcoordRight.X( );
        --hexcoordRight.Y( );
        break;


    default:
        ASSERT( 0 );
    }

#ifdef _DEBUG
    CheckPt( pt, hexcoord );
#endif

    CBuilding* pbuildingPrev = NULL;

    CHexCoordColumnIter hexiterLeft( this, hexcoordLeft, ptLeft, TRUE );
    CHexCoordColumnIter hexiterRight( this, hexcoordRight, ptRight, TRUE );

    CHexCoordColumnIter* aphexiter[3] = { &hexiter, &hexiterLeft, &hexiterRight };

    CDrawInfoPool drawinfopool;

    // Walk 3 columns (left and right columns to catch overhanging vehicles)
    // Check for building only in middle column

    for ( int i = 0; i < 3; ++i )
    {
        // Start higher to catch overhanging vehicles

        aphexiter[i]->ToPrev( );
        aphexiter[i]->ToPrev( );

        for ( ; aphexiter[i]->Value( ); aphexiter[i]->ToNext( ) )
        {
            hexcoord = *aphexiter[i]->Value( );
            phex     = theMap.GetHex( hexcoord );

            CHexCoord hexcoordWrapped = hexcoord;

            hexcoordWrapped.Wrap( );

            ASSERT_STRICT_VALID( phex );

            BYTE byUnits = phex->GetUnits( );

            // Check for a building if walking the column where the cursor is

            if ( 0 == i && ( byUnits & CHex::bldg ) )
            {
                CBuilding* pbuilding = phex->GetVisibleBuilding( hexcoordWrapped );

                if ( pbuilding && pbuilding != pbuildingPrev )
                {
                    CHexCoord hexcoordCorner = pbuilding->GetLeftHex( m_iDir );

                    hexcoordCorner.ToNearestHex( hexcoord );

                    if ( pbuilding->IsHit( hexcoordCorner, pt ) )
                    {
                        btreetiledrawinfo.Insert( drawinfopool.GetStructureDrawInfo(
                            pbuilding, CTileDrawInfo::building, CHexCoord( ), pbuilding->GetMapLoc( ),
                            CStructureSprite::FOREGROUND_LAYER ) );

                        pbuildingPrev = pbuilding;
                    }
                }
            }

            // Check for a bridge if walking the column where the cursor is

            else if ( 0 == i && ( byUnits & CHex::bridge ) )
            {
                CBridgeUnit* pbridgeunit = theBridgeHex.GetBridge( hexcoordWrapped );

                if ( pbridgeunit )
                {
                    if ( pbridgeunit->IsHit( hexcoord, pt ) )
                    {
                        CMapLoc maploc( hexcoord );

                        maploc.x += 32;
                        maploc.y += 32;

                        btreetiledrawinfo.Insert(
                            drawinfopool.GetStructureDrawInfo( pbridgeunit, CTileDrawInfo::bridge, hexcoord, maploc,
                                                               CStructureSprite::FOREGROUND_LAYER ) );
                    }
                }
            }

            // Check for a vehicle

            BOOL bVehicle = 0 == ( byUnits & CHex::bldg ) && 0 != ( byUnits & CHex::veh );

            if ( bVehicle )
            {
                bVehicle = FALSE;

                CVehicle* pvehicle = NULL;
                CSubHex   subhex( hexcoordWrapped );
                int       x = subhex.x;
                int       y = subhex.y;

                for ( int j = 0; j < 4; ++j )
                {
                    switch ( j )
                    {
                    case 0:
                        bVehicle = 0 != ( byUnits & CHex::ul );
                        break;

                    case 1:
                        bVehicle = 0 != ( byUnits & CHex::ur );
                        subhex   = CSubHex( x + 1, y );
                        break;

                    case 2:
                        bVehicle = 0 != ( byUnits & CHex::lr );
                        subhex   = CSubHex( x + 1, y + 1 );
                        break;

                    case 3:
                        bVehicle = 0 != ( byUnits & CHex::ll );
                        subhex   = CSubHex( x, y + 1 );
                        break;

                    default:
                        ASSERT( 0 );
                    }

                    ASSERT( FALSE == bVehicle || TRUE == bVehicle );

                    if ( bVehicle )
                    {
                        pvehicle = theVehicleHex.GetVehicle( subhex );

                        ASSERT( pvehicle );

                        if ( ( pvehicle != NULL ) && pvehicle->IsVisible( ) && pvehicle->IsHit( CHexCoord( ), pt ) )
                            btreetiledrawinfo.Insert( drawinfopool.GetVehicleDrawInfo( pvehicle, CHexCoord( ) ) );
                    }
                }

                ASSERT( pvehicle );  // There was supposed to be a vehicle on at least one subhex
            }
        }
    }

    // Find the closest vehicle or building

    BTreeIter<CTileDrawInfo> iter( &btreetiledrawinfo );

    CTileDrawInfo* pdrawinfo = iter.ToLast( );

    if ( pdrawinfo )
        if ( CTileDrawInfo::vehicle == pdrawinfo->GetTileType( ) )
            hitinfo.SetVehicleHit( (CVehicle*)pdrawinfo->GetTile( ) );
        else if ( CTileDrawInfo::bridge == pdrawinfo->GetTileType( ) )
            hitinfo.SetBridgeHit( (CBridgeUnit*)pdrawinfo->GetTile( ) );
        else
        {
            ASSERT( CTileDrawInfo::building == pdrawinfo->GetTileType( ) );

            hitinfo.SetBuildingHit( (CBuilding*)pdrawinfo->GetTile( ) );
        }

    return hitinfo;
}

//--------------------------------------------------------------------------
// CAnimAtr::WindowToView - Convert Window coord to view coord
//--------------------------------------------------------------------------
CPoint CAnimAtr::WindowToView( CPoint pt ) const
{
    return CPoint( pt.x + m_ptUL.x, pt.y + m_ptUL.y );
}

//--------------------------------------------------------------------------
// CAnimAtr::WindowToMap - Convert Window coords to map coords
//--------------------------------------------------------------------------
CMapLoc CAnimAtr::WindowToMap( CPoint pt, Fixed fixZ ) const
{
    pt = WindowToView( pt );

    return ViewToMap( pt, fixZ );
}

//--------------------------------------------------------------------------
// CAnimAtr::ViewToMap - Convert View coords to map coords
//--------------------------------------------------------------------------
CMapLoc CAnimAtr::ViewToMap( CPoint pt, Fixed fixZ ) const
{
    pt.x <<= m_iZoom;
    pt.y <<= m_iZoom;

    pt.y += ( fixZ << TERRAIN_HT_SHIFT ).Round( );

    switch ( m_iDir )
    {
    case 0:
        pt = CPoint( ( pt.x >> 1 ) - pt.y, ( pt.x >> 1 ) + pt.y );
        break;

    case 1:
        pt = CPoint( -( pt.x >> 1 ) - pt.y, ( pt.x >> 1 ) - pt.y );
        break;

    case 2:
        pt = CPoint( -( pt.x >> 1 ) + pt.y, -( pt.x >> 1 ) - pt.y );
        break;

    case 3:
        pt = CPoint( ( pt.x >> 1 ) + pt.y, -( pt.x >> 1 ) + pt.y );
        break;

    default:
        ASSERT( 0 );
    }

    return pt;
}

//--------------------------------------------------------------------------
// CAnimAtr::_WindowToHex - Convert Window coord to CHexCoord
//--------------------------------------------------------------------------
CHexCoord CAnimAtr::_WindowToHex( CPoint pt ) const
{
    Fixed fixZ = m_maploc.CalcAltitude( );

    CMapLoc   maploc( WindowToMap( pt, fixZ ) );
    CHexCoord hexcoord( maploc );

    CHexCoordColumnIter hexiter( this, hexcoord, pt );

    CRect rect( 0, 0, 0, 0 );

    for ( ;; hexiter.ToNext( ) )
    {
        if ( !IsHexHit( *hexiter.Value( ), pt, rect ) && pt.y <= rect.top )
            break;
    }

    for ( hexiter.ToPrev( );; hexiter.ToPrev( ) )
        if ( IsHexHit( *hexiter.Value( ), pt, rect ) || pt.y > rect.bottom )
            break;

    return *hexiter.Value( );
}

//--------------------------------------------------------------------------
// CAnimAtr::WindowToSubHex - Convert Window coord to CSubHex
//--------------------------------------------------------------------------
CSubHex CAnimAtr::WindowToSubHex( CPoint pt ) const
{
    CHexCoord hexcoord = WindowToHex( pt );

    // FIXIT: narrow search to the subhex

    return CSubHex( hexcoord.X( ) << 1, hexcoord.Y( ) << 1 );
}

//---------------------------------------------------------------------------
// CMapLoc::ToNearest - Assigns the copy of this maploc closest to the specified maploc
//---------------------------------------------------------------------------
CMapLoc& CMapLoc::ToNearest( const CMapLoc& maploc )
{
    int iWidth   = theMap.Get_eX( ) << HEX_HT_PWR;
    int iHeight  = theMap.Get_eY( ) << HEX_HT_PWR;
    int iWidth2  = iWidth >> 1;
    int iHeight2 = iHeight >> 1;

    while ( x - maploc.x > iWidth2 ) x -= iWidth;

    while ( x - maploc.x < -iWidth2 ) x += iWidth;

    while ( y - maploc.y > iHeight2 ) y -= iHeight;

    while ( y - maploc.y < -iHeight2 ) y += iHeight;

    return *this;
}

//--------------------------------------------------------------------------
// CMapLoc::CalcAltitude - Get interpolated altitude of hex at offset
//--------------------------------------------------------------------------
Fixed  // Altitude interpolated from corner hexes
CMapLoc::CalcAltitude( ) const
{
    int iU  = HexOffX( );
    int iU2 = MAX_HEX_HT - iU;
    int iT  = HexOffY( );
    int iT2 = MAX_HEX_HT - iT;

    ASSERT_STRICT( iU + iU2 == MAX_HEX_HT );
    ASSERT_STRICT( iT + iT2 == MAX_HEX_HT );

    CHex*     aphex[4];
    CHexCoord hexcoord( *this );

    hexcoord.GetHexCorners( aphex );

    int iRet = iT2 * iU2 * aphex[0]->GetAltDraw( ) + iT2 * iU * aphex[1]->GetAltDraw( ) +
               iT * iU * aphex[2]->GetAltDraw( ) + iT * iU2 * aphex[3]->GetAltDraw( );

    Fixed fix;

    ASSERT_STRICT( 16 >= HEX_HT_PWR_2 );

    fix.Value( iRet << 16 - HEX_HT_PWR_2 );

    return fix;
}

//--------------------------------------------------------------------------
// CMapLoc::CalcNormal - Get approximate normal vector at sub-hex coord
//
// Returns the (x, y) component of the normal vector at the nearest vertex
//--------------------------------------------------------------------------
CPoint  // Normal (x, y), z-component is always MAX_HEX_HT
CMapLoc::CalcNormal( ) const
{
    CPoint ptNormal;

#define INTERPOLATED_NORMAL 0
#if INTERPOLATED_NORMAL

    int       aaiAlt[4][4];
    int       x = HexX( );
    int       y = HexY( ) - 1;
    CHexCoord hexcoord;

    for ( int i = 0; i < 4; ++i )
        for ( int j = 0; j < 4; ++j )
        {
            hexcoord = CHexCoord( x + i - 1, y + j - 1 );

            aaiAlt[i][j] = theMap.GetHex( hexcoord )->GetAltDraw( );
        }

    CPoint ptUL = CPoint( ( aaiAlt[0][1] - aaiAlt[2][1] ) >> 1, ( aaiAlt[1][0] - aaiAlt[1][2] ) >> 1 );

    CPoint ptUR = CPoint( ( aaiAlt[1][1] - aaiAlt[3][1] ) >> 1, ( aaiAlt[2][0] - aaiAlt[2][2] ) >> 1 );

    CPoint ptLR = CPoint( ( aaiAlt[1][2] - aaiAlt[3][2] ) >> 1, ( aaiAlt[2][1] - aaiAlt[2][3] ) >> 1 );

    CPoint ptLL = CPoint( ( aaiAlt[0][2] - aaiAlt[2][2] ) >> 1, ( aaiAlt[1][1] - aaiAlt[1][3] ) >> 1 );

    int iU  = HexOffX( );
    int iU2 = MAX_HEX_HT - iU;
    int iT  = HexOffY( );
    int iT2 = MAX_HEX_HT - iT;

    ASSERT_STRICT( iU + iU2 == MAX_HEX_HT );
    ASSERT_STRICT( iT + iT2 == MAX_HEX_HT );

    int i1 = iT2 * iU2;
    int i2 = iT2 * iU;
    int i3 = iT * iU;
    int i4 = iT * iU2;

    ptNormal = CPoint( ( i1 * ptUL.x + i2 * ptUR.x + i3 * ptLR.x + i4 * ptLL.x ) >> HEX_HT_PWR_2,
                       ( i1 * ptUL.y + i2 * ptUR.y + i3 * ptLR.y + i4 * ptLL.y ) >> HEX_HT_PWR_2 );

#else

    //	static	int	xaaiIndex[2][2] = {{ 0, 3 },
    //												 { 1, 2 }};

    static int xaaiIndex[2][2] = { { 0, 1 }, { 3, 2 } };

    CHex*     aphex[4];
    CPoint    ptN;
    CHexCoord hexcoord( *this );

    hexcoord.GetHexCorners( aphex );

    switch ( xaaiIndex[HexOffX( ) >> HEX_HT_PWR - 1][HexOffY( ) >> HEX_HT_PWR - 1] )
    {
    case 0:
        ptNormal = CPoint( aphex[0]->GetAltDraw( ) - aphex[1]->GetAltDraw( ),
                           aphex[0]->GetAltDraw( ) - aphex[3]->GetAltDraw( ) );

        break;

    case 1:
        ptNormal = CPoint( aphex[0]->GetAltDraw( ) - aphex[1]->GetAltDraw( ),
                           aphex[1]->GetAltDraw( ) - aphex[2]->GetAltDraw( ) );

        break;

    case 2:
        ptNormal = CPoint( aphex[3]->GetAltDraw( ) - aphex[2]->GetAltDraw( ),
                           aphex[1]->GetAltDraw( ) - aphex[2]->GetAltDraw( ) );

        break;

    case 3:
        ptNormal = CPoint( aphex[3]->GetAltDraw( ) - aphex[2]->GetAltDraw( ),
                           aphex[0]->GetAltDraw( ) - aphex[3]->GetAltDraw( ) );

        break;

    default:
        ASSERT( 0 );
    }

#endif

    return ptNormal;
}

//--------------------------------------------------------------------------
// CHexCoord::GetHexCorners - Get array of pointers to CHexs at corners
//
//	   -----------> x		Hexes are returned in this order (x,y world axes)
//	  |0		1|
//	  |		 |
//	  |  o	 |		o = CMapLoc coordinates
//	  |		 |
//	  |3		2|
//	  |-------
//	  |
//	  v
//	  y
//
//--------------------------------------------------------------------------
void CHexCoord::GetHexCorners( CHex* aphex[4] )  // Output array of CHex pointers
    const                                        // Order is view-independent.
{
    CHexCoord hexcoord1( *this );
    CHexCoord hexcoord2( hexcoord1.X( ) + 1, hexcoord1.Y( ) );
    CHexCoord hexcoord3( hexcoord1.X( ) + 1, hexcoord1.Y( ) + 1 );
    CHexCoord hexcoord4( hexcoord1.X( ), hexcoord1.Y( ) + 1 );

    aphex[0] = theMap.GetHex( hexcoord1 );
    aphex[1] = theMap.GetHex( hexcoord2 );
    aphex[2] = theMap.GetHex( hexcoord3 );
    aphex[3] = theMap.GetHex( hexcoord4 );
}

//--------------------------------------------------------------------------
// CHexCoord::GetWorldHex - Get 3D world coords of 4 corners of a hex
//
//	   -----------> x		Points are returned in this order (x,y world axes)
//	  |0		1|
//	  |		 |
//	  |  o	 |		o = CMapLoc coordinates
//	  |		 |
//	  |3		2|
//	  |-------
//	  |
//	  v
//	  y
//
//--------------------------------------------------------------------------
void CHexCoord::GetWorldHex( CMapLoc3D amaploc3d[4] )  // Output in 3D world map coords
    const
{
    CHex* aphex[4];

    GetHexCorners( aphex );

    for ( int i = 0; i < 4; ++i ) amaploc3d[i].m_fixZ = aphex[i]->GetAltDraw( );

    CMapLoc maploc( *this );

    amaploc3d[0].x = maploc.x;
    amaploc3d[0].y = maploc.y;
    amaploc3d[1].x = maploc.x + MAX_HEX_HT;
    amaploc3d[1].y = maploc.y;
    amaploc3d[2].x = maploc.x + MAX_HEX_HT;
    amaploc3d[2].y = maploc.y + MAX_HEX_HT;
    amaploc3d[3].x = maploc.x;
    amaploc3d[3].y = maploc.y + MAX_HEX_HT;
}

//--------------------------------------------------------------------------
// CMapLoc3D::CMapLoc3D	- Convert left corner of hex to world coords
//--------------------------------------------------------------------------
CMapLoc3D::CMapLoc3D( const CHexCoord& rhexcoord ): CMapLoc( rhexcoord )
{
    m_fixZ = theMap.GetHex( CHexCoord( *this ) )->GetAltDraw( );
}

//--------------------------------------------------------------------------
// CAnimAtr::WorldToView - Transform 3D world coordinates to 2D view coordinates
//--------------------------------------------------------------------------
CPoint  // View coordinates
CAnimAtr::WorldToView( const CMapLoc3D& maploc3d ) const
{
    int iNewX, iNewY;

    switch ( m_iDir )
    {
    case 0:
        iNewX = maploc3d.x + maploc3d.y;
        iNewY = ( -maploc3d.x + maploc3d.y ) >> 1;
        break;

    case 1:
        iNewX = -maploc3d.x + maploc3d.y;
        iNewY = ( -maploc3d.x - maploc3d.y ) >> 1;
        break;

    case 2:
        iNewX = -maploc3d.x - maploc3d.y;
        iNewY = ( maploc3d.x - maploc3d.y ) >> 1;
        break;

    case 3:
        iNewX = maploc3d.x - maploc3d.y;
        iNewY = ( maploc3d.x + maploc3d.y ) >> 1;
        ;
        break;
    }

    Fixed fixY = maploc3d.m_fixZ << TERRAIN_HT_SHIFT;

    iNewY -= fixY.Round( );

    return CPoint( iNewX >> m_iZoom, iNewY >> m_iZoom );
}

#ifdef _DEBUG
#ifdef BUGBUG
void CHexCoord::AssertValid( ) const
{
    //	if (theMap.m_eX == 0)
    //		return;

    // GG: Don't always want these wrapped unless they're used to index
    //		 into the map. This gives correct transformations to other
    //		 coordinate systems such as view and ViewHex.

    //	ASSERT ((0 <= m_iX) && (m_iX < theMap.m_eX));
    //	ASSERT ((0 <= m_iY) && (m_iY < theMap.m_eY));
}
#endif

void CHexCoord::AssertStrictValid( ) const
{
    if ( theMap.m_eX == 0 )
        return;

    ASSERT( ( 0 <= m_iX ) && ( m_iX < theMap.m_eX ) );
    ASSERT( ( 0 <= m_iY ) && ( m_iY < theMap.m_eY ) );
}

void CSubHex::AssertValid( ) const
{

    if ( theMap.m_eX == 0 )
        return;

    ASSERT( ( 0 <= x ) && ( x < theMap.m_eX * 2 ) );
    ASSERT( ( 0 <= y ) && ( y < theMap.m_eY * 2 ) );
}

//--------------------------------------------------------------------------
// CAnimAtr::AssertValid
//--------------------------------------------------------------------------
void CAnimAtr::AssertValid( ) const
{
    ASSERT( m_ptUL.x == WorldToView( CMapLoc3D( m_maploc ) ).x - ( m_dibwnd.GetWinSize( ).cx >> 1 ) );
    ASSERT( m_ptUL.y == WorldToView( CMapLoc3D( m_maploc ) ).y - ( m_dibwnd.GetWinSize( ).cy >> 1 ) );

    ASSERT_VALID( &m_dibwnd );
    ASSERT( ( 0 <= m_iDir ) && ( m_iDir < NUM_BLDG_DIRECTIONS ) );
    ASSERT( ( theApp.GetZoomData( )->GetFirstZoom( ) <= m_iZoom ) && ( m_iZoom < NUM_ZOOM_LEVELS ) );
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CTerrain - data on terrain hexes

#ifdef _DEBUG
void CTerrainData::AssertValid( ) const
{

    // assert the base class
    CObject::AssertValid( );

    ASSERT( ( 0 <= m_iBuildMult ) && ( m_iBuildMult <= 24 ) );
    ASSERT( ( 0 <= m_iFarmMult ) && ( m_iFarmMult <= 20 ) );
    for ( int iInd = 0; iInd < NUM_WHEEL_TYPES; iInd++ )
    {
        ASSERT( ( 0 <= m_iWheelMult[iInd] ) && ( m_iWheelMult[iInd] <= 42 ) );
        ASSERT( ( 0 < m_iDefenseMult[iInd] ) && ( m_iDefenseMult[iInd] <= MAX_DEF_MULT ) );
    }
    ASSERT_VALID_CSTRING( &m_sDesc );
}

void CTerrain::AssertValid( ) const
{

    // assert the base class
    CSpriteStore<CTerrainSprite>::AssertValid( );

    ASSERT( this == &theTerrain );

    // if not init'ed yet, return
    if ( GetCount( ) == 0 )
        return;

    CTerrainData const* pTd = m_Data;
    for ( int iOn = 0; iOn < CHex::num_types; iOn++, pTd++ ) ASSERT_VALID( pTd );
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CHex - a single hex

//--------------------------------------------------------------------------
// CHex::Draw
//--------------------------------------------------------------------------
CRect CHex::Draw( const CHexCoord& hexcoord )
{
    int iType = m_psprite->GetID( );

    // Make sure triangle halves match?

    BOOL bDrawVert = road == iType || resources == iType;

    // Use shading? (fog-of-war still applies)

    BOOL bShade =
        river != iType && coastline != iType && swamp != iType && ocean != iType && resources != iType && lake != iType;

    CTerrainDrawParms::FEATHER_TYPE aeFeather[4] = { CTerrainDrawParms::FEATHER_NONE, CTerrainDrawParms::FEATHER_NONE,
                                                     CTerrainDrawParms::FEATHER_NONE, CTerrainDrawParms::FEATHER_NONE };

    BOOL bCursor = no_cur != GetCursorMode( );

    // Don't feather any neighbor if this is a road, city terrain, building cursor, or has a building

    if ( road != iType && resources != iType && city != iType && !bCursor && GetVisibleBuilding( hexcoord ) == NULL )
    {
        int iX = hexcoord.X( );
        int iY = hexcoord.Y( );

        static int x_aaiOffset[4][4][2] =  // dir x neighbor (TL,BL,TR,BR) x (x,y)
            {
                //	  TL           BL          TR         BR
                { { 0, -1 }, { -1, 0 }, { 1, 0 }, { 0, 1 } },  // dir = 0
                { { 1, 0 }, { 0, -1 }, { 0, 1 }, { -1, 0 } },  // dir = 1
                { { 0, 1 }, { 1, 0 }, { -1, 0 }, { 0, -1 } },  // dir = 2
                { { -1, 0 }, { 0, 1 }, { 0, -1 }, { 1, 0 } },  // dir = 3
            };

        for ( int i = 0; i < 4; ++i )
        {
            CHexCoord hexcoordNeighbor( iX + x_aaiOffset[xiDir][i][0], iY + x_aaiOffset[xiDir][i][1] );

            CHex* phexNeighbor    = theMap.GetHex( hexcoordNeighbor );
            int   iTypeNeighbor   = phexNeighbor->m_psprite->GetID( );
            BOOL  bCursorNeighbor = no_cur != phexNeighbor->GetCursorMode( );

            // Don't feather this edge if neighbor has a building cursor, road, a visible building, a resource, a city
            // tile

            if ( bCursorNeighbor || road == iTypeNeighbor || city == iTypeNeighbor || resources == iTypeNeighbor ||
                 phexNeighbor->GetVisibleBuilding( hexcoordNeighbor ) != NULL )
                continue;

            // Feather in coastline edges bordering non-coastline tiles

            if ( coastline == iType && coastline != iTypeNeighbor )
                aeFeather[i] = CTerrainDrawParms::FEATHER_IN;

            // Feather out edge with coastline, unless this tile is coastline

            else if ( coastline == iTypeNeighbor && coastline != iType )
                aeFeather[i] = CTerrainDrawParms::FEATHER_OUT;

            // Everything else is in-out feathering

            else if ( coastline != iTypeNeighbor && coastline != iType )
                aeFeather[i] = CTerrainDrawParms::FEATHER_INOUT;
        }
    }

    CTerrainDrawParms drawparms( *this, hexcoord, bDrawVert, bShade, aeFeather );

    CRect rectBound = GetSprite( )->GetView( xiDir, 0 )->Draw( drawparms );

    if ( CDrawParms::IsInvalidateMode( ) )
    {
        if ( hexcoord.IsInvalidated( ) )  // Flag cleared in mainloop
            xpanimatr->GetDirtyRects( )->AddRect( &rectBound );

        if ( bCursor )
            xpanimatr->GetDirtyRects( )->AddRect( &rectBound, CDirtyRects::RECT_LIST::LIST_PAINT_BOTH );
    }

    return rectBound;
}

//--------------------------------------------------------------------------
// CHex::GetVisibleBuilding
//--------------------------------------------------------------------------
CBuilding* CHex::GetVisibleBuilding( CHexCoord hexcoord ) const
{
    if ( 0 == ( bldg & GetUnits( ) ) )
        return NULL;

    hexcoord.Wrap( );

    CBuilding* pbuilding = theBuildingHex._GetBuilding( hexcoord );

    if ( !pbuilding->IsVisible( ) )
        return NULL;

    return pbuilding;
}

CString CHex::GetStatus( )
{

    ASSERT_VALID( this );
    return ( theTerrain.GetData( GetVisibleType( ) ).GetDesc( ) );
}

// change to road hex, call for any neighbors because we can change them
void CHex::ChangeToRoad( CHexCoord& hex, BOOL bCallNext, BOOL bForce )
{

    ASSERT_VALID( this );

    // if we don't see it yet, just mark it
    m_bType = CHex::road;
    if ( ( !GetVisibility( ) ) && ( !bForce ) && ( GetVisibleType( ) != road ) )
        return;

    SetTree( 0 );
    SetVisibleType( road );

    // we are going to create an 8-bit number that tells us who is around us
    int iType = 0;

    //   above
    CHexCoord _hexOn( hex.X( ), hex.Y( ) - 1 );
    _hexOn.Wrap( );
    CHex* pHex = theMap._GetHex( _hexOn );
    if ( pHex->GetType( ) == CHex::road )
    {
        iType |= 1;
        if ( bCallNext )
            pHex->ChangeToRoad( _hexOn, FALSE );
    }
    else if ( pHex->GetUnits( ) & CHex::bldg )
    {
        CBuilding* pBldg = theBuildingHex.GetBuilding( _hexOn );
        if ( pBldg != NULL )
            if ( pBldg->GetExitHex( ) == _hexOn )
                iType |= 1;
    }

    //   left
    _hexOn.Xdec( );
    _hexOn.Yinc( );
    pHex = theMap._GetHex( _hexOn );
    if ( pHex->GetType( ) == CHex::road )
    {
        iType |= 2;
        if ( bCallNext )
            pHex->ChangeToRoad( _hexOn, FALSE );
    }
    else if ( pHex->GetUnits( ) & CHex::bldg )
    {
        CBuilding* pBldg = theBuildingHex.GetBuilding( _hexOn );
        if ( pBldg != NULL )
            if ( pBldg->GetExitHex( ) == _hexOn )
                iType |= 2;
    }

    //   right
    _hexOn.X( ) += 2;
    _hexOn.Wrap( );
    pHex = theMap._GetHex( _hexOn );
    if ( pHex->GetType( ) == CHex::road )
    {
        iType |= 4;
        if ( bCallNext )
            pHex->ChangeToRoad( _hexOn, FALSE );
    }
    else if ( pHex->GetUnits( ) & CHex::bldg )
    {
        CBuilding* pBldg = theBuildingHex.GetBuilding( _hexOn );
        if ( pBldg != NULL )
            if ( pBldg->GetExitHex( ) == _hexOn )
                iType |= 4;
    }

    //   bottom
    _hexOn.Xdec( );
    _hexOn.Yinc( );
    pHex = theMap._GetHex( _hexOn );
    if ( pHex->GetType( ) == CHex::road )
    {
        iType |= 8;
        if ( bCallNext )
            pHex->ChangeToRoad( _hexOn, FALSE );
    }
    else if ( pHex->GetUnits( ) & CHex::bldg )
    {
        CBuilding* pBldg = theBuildingHex.GetBuilding( _hexOn );
        if ( pBldg != NULL )
            if ( pBldg->GetExitHex( ) == _hexOn )
                iType |= 8;
    }

    int iIndex;
    switch ( iType )
    {
        // case 0 - default

    case 0x1:  // above
    case 0x8:  // below
    case 0x9:  // above & below
        iIndex = CHex::r_vert;
        break;
    case 0x2:  // left
    case 0x4:  // right
    case 0x6:  // laft & right
        iIndex = CHex::r_horz;
        break;

    case 0x3:  // L lr
        iIndex = CHex::r_l_lr;
        break;
    case 0x5:  // L ll
        iIndex = CHex::r_l_ll;
        break;
    case 0xA:  // L lr
        iIndex = CHex::r_l_ur;
        break;
    case 0xC:  // L ul
        iIndex = CHex::r_l_ul;
        break;

    case 0x7:  // T down
        iIndex = CHex::r_t_dn;
        break;
    case 0xB:  // T right
        iIndex = CHex::r_t_rt;
        break;
    case 0xD:  // T left
        iIndex = CHex::r_t_lf;
        break;
    case 0xE:  // T up
        iIndex = CHex::r_t_up;
        break;

    // case 0xF
    default:  // no neighbors (0), all neighbors (0xF), bugs
        iIndex = CHex::r_x;
        break;
    }

    // set the new sprite
    m_psprite = theTerrain.GetSprite( CHex::road, iIndex );

    // invalidate it so it redraws
    hex.SetInvalidated( );
    theApp.m_wndWorld.NewMode( );
}

#ifdef _DEBUG
void CHex::AssertValid( ) const
{

    // assert base object
    CSimpleTile::AssertValid( );

    if ( theMap.m_pHex == NULL )
        return;

    ASSERT( ( 0 <= GetType( ) ) && ( GetType( ) < CHex::num_types ) );

    long lOff = theMap.GetHexOff( this );
    ASSERT( lOff < (long)theMap.m_eX * (long)theMap.m_eY );
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CGameMap - the collection of hexes

// returns the distance between units in number of CHexCoords
//   returns - (distance) if one can't see the other
int CGameMap::LineOfSight( CUnit const* pSrc, CUnit const* pDest ) const
{

    ASSERT_VALID( pSrc );
    ASSERT_VALID( pDest );

    // we walk from one to the other checking for blocking buildings or terrain.
    // vehicles NEVER block
    CSubHex _subSrc( pSrc->GetWorldPixels( ) );
    _subSrc.Wrap( );
    CSubHex _subDest( pDest->GetWorldPixels( ) );
    _subDest.Wrap( );
    int  iSrcType  = pSrc->GetUnitType( );
    int  iDestType = pDest->GetUnitType( );
    int  iDist     = 0;
    BOOL bBlocked  = FALSE;

    // these are used for terrain blocking
    CHexCoord _hexOn( _subSrc );
    CHexCoord _hexPrev( _hexOn );
    CHexCoord _hexPrevPrev( _hexOn );
    CHex*     pHex = theMap.GetHex( _hexOn );

    int dx = CSubHex::Diff( _subDest.x - _subSrc.x );
    int ax = abs( dx ) << 1;
    int sx = ( dx < 0 ) ? -1 : 1;
    int dy = CSubHex::Diff( _subDest.y - _subSrc.y );
    int ay = abs( dy ) << 1;
    int sy = ( dy < 0 ) ? -1 : 1;
    int d;
    if ( ax > ay )
        d = ay - ( ax >> 1 );
    else
        d = ax - ( ay >> 1 );

    // if real far - don't check
    if ( ax + ay > CVehicle::m_iMaxRange * 2 )
        return -( ax + ay ) / 2;

    // only need to test for on source till not on
    BOOL bTestOnSrc = TRUE;

    while ( TRUE )
    {
        // get the next sub-hex we walk to
        int iAddDist = 2;
        if ( ax > ay )
        {
            if ( d >= 0 )
            {
                _subSrc.y += sy;
                _subSrc.WrapY( );
                d -= ax;
                iAddDist++;
            }
            _subSrc.x += sx;
            _subSrc.WrapX( );
            d += ay;
        }
        else
        {
            if ( d >= 0 )
            {
                _subSrc.x += sx;
                _subSrc.WrapX( );
                d -= ay;
                iAddDist++;
            }
            _subSrc.y += sy;
            _subSrc.WrapY( );
            d += ax;
        }

        // see if done
        if ( _subSrc == _subDest )
            break;

        // inc the CHexCoords
        if ( !_hexOn.SameHex( _subSrc ) )
        {
            _hexPrevPrev = _hexPrev;
            _hexPrev     = _hexOn;
            _hexOn       = _subSrc;

            // get the hex we are on
            pHex = theMap.GetHex( _hexOn );
        }

        // a bunch of tests we only hit if there is a unit here
        if ( pHex->GetUnits( ) & CHex::unit )
        {
            BYTE bVeh = CHex::GetVehUnits( _subSrc );
            TRAP( ( bVeh != 1 ) && ( bVeh != 2 ) && ( bVeh != 4 ) && ( bVeh != 8 ) );

            // see if done
            if ( iDestType == CUnit::vehicle )
            {
                if ( pHex->GetUnits( ) & bVeh )
                    if ( theVehicleHex._GetVehicle( _subSrc ) == pDest )
                        break;
            }
            else if ( pHex->GetUnits( ) & CHex::bldg )
                if ( theBuildingHex._GetBuilding( _subSrc ) == pDest )
                    break;

            // if we are still on ourself we don't do anything
            if ( bTestOnSrc )
            {
                if ( iSrcType == CUnit::vehicle )
                {
                    if ( pHex->GetUnits( ) & bVeh )
                        if ( theVehicleHex._GetVehicle( _subSrc ) == pSrc )
                            continue;
                    bTestOnSrc = FALSE;
                }
                else if ( pHex->GetUnits( ) & CHex::bldg )
                {
                    if ( theBuildingHex._GetBuilding( _subSrc ) == pSrc )
                        continue;
                    bTestOnSrc = FALSE;
                }
                else
                    bTestOnSrc = FALSE;
            }
        }

        // add in the distance
        iDist += iAddDist;

        // figure blocking
        if ( !bBlocked )
        {
            if ( pHex->GetUnits( ) & CHex::bldg )
                bBlocked = TRUE;
            else

                // we need to insure _hexPrev is not on pSrc
                if ( _hexPrevPrev != _hexPrev )
                {
                    // trees block - not infantry
                    if ( pHex->GetType( ) == CHex::forest )
                    {
                        if ( pSrc->GetUnitType( ) != CUnit::vehicle )
                            bBlocked = TRUE;
                        else if ( ( (CVehicle*)pSrc )->GetData( )->GetBaseType( ) != CTransportData::infantry )
                            bBlocked = TRUE;
                    }
                    else

                    // the hex before and after must be lower (we're on a peak)
                    {
                        int iAlt = theMap.GetHex( _hexPrev )->GetAlt( );
                        if ( ( iAlt - theMap.GetHex( _hexPrevPrev )->GetAlt( ) > LOS_ALT ) &&
                             ( iAlt - pHex->GetAlt( ) > LOS_ALT ) )
                            bBlocked = TRUE;
                    }
                }
        }
    }

    // we round up and divide because a distance of 4.5 requires a vehicle with
    // a range of 5, not 4.
    // we divide by 2 because we added 2,3 instead of 1,1.5
    // we then divide by 2 to convert from CSubHex to CHexCoord
    iDist = ( iDist + 3 ) / 4;
    if ( bBlocked )
        iDist = -iDist;
    return ( iDist );
}

// direction multipliers (Coord On direction_on_map)
//  order in array xOnX, yOnX, xOnY, yOnY;
void CGameMap::DirMult( int iDir, int* pInt ) const
{

    switch ( iDir )
    {
    case 0:
        pInt[0] = pInt[1] = pInt[3] = 1;
        pInt[2]                     = -1;
        break;
    case 1:
        pInt[0] = pInt[2] = pInt[3] = -1;
        pInt[1]                     = 1;
        break;
    case 2:
        pInt[0] = pInt[1] = pInt[3] = -1;
        pInt[2]                     = 1;
        break;
    case 3:
        pInt[0] = pInt[2] = pInt[3] = 1;
        pInt[1]                     = -1;
        break;
    }
}

// return distance in CHexCoord's
int CGameMap::GetRangeDistance( CUnit const* pSrc, CUnit const* pDest ) const
{

    // SubHex they are centered on
    CSubHex _subSrc( pSrc->GetWorldPixels( ) );
    CSubHex _subDest( pDest->GetWorldPixels( ) );

    // move toward the other while still on SubHexes they own
    int dX   = CSubHex::Diff( _subDest.x - _subSrc.x );
    int iMax = abs( dX );
    dX       = dX < -1 ? -1 : ( dX > 1 ? 1 : dX );
    int dY   = CSubHex::Diff( _subDest.y - _subSrc.y );
    iMax     = __max( abs( dY ), iMax ) + 1;
    dY       = dY < -1 ? -1 : ( dY > 1 ? 1 : dY );

    // move src
    if ( pSrc->GetUnitType( ) == CUnit::building )
    {
        while ( iMax > 0 )
        {
            CSubHex _subTmp( _subSrc );
            _subTmp.x += dX;
            _subTmp.y += dY;
            _subTmp.Wrap( );
            if ( theBuildingHex.GetBuilding( _subTmp ) != pSrc )
                break;
            _subSrc = _subTmp;
            iMax--;
        }
    }
    else
    {
        CSubHex _subTmp( _subSrc );
        _subTmp.x += dX;
        _subTmp.y += dY;
        _subTmp.Wrap( );
        if ( ( _subTmp == ( (CVehicle*)pSrc )->GetPtHead( ) ) || ( _subTmp == ( (CVehicle*)pSrc )->GetPtTail( ) ) ||
             ( _subTmp == ( (CVehicle*)pSrc )->GetPtNext( ) ) )
            _subSrc = _subTmp;
    }

    // move dest
    if ( pDest->GetUnitType( ) == CUnit::building )
    {
        while ( iMax > 0 )
        {
            CSubHex _subTmp( _subDest );
            _subTmp.x += dX;
            _subTmp.y += dY;
            _subTmp.Wrap( );
            if ( theBuildingHex.GetBuilding( _subTmp ) != pSrc )
                break;
            _subDest = _subTmp;
            iMax--;
        }
    }
    else
    {
        CSubHex _subTmp( _subDest );
        _subTmp.x -= dX;
        _subTmp.y -= dY;
        _subTmp.Wrap( );
        if ( ( _subTmp == ( (CVehicle*)pDest )->GetPtHead( ) ) || ( _subTmp == ( (CVehicle*)pDest )->GetPtTail( ) ) ||
             ( _subTmp == ( (CVehicle*)pDest )->GetPtNext( ) ) )
            _subDest = _subTmp;
    }

    // get the distance from the closest points
    dX = abs( CSubHex::Diff( _subDest.x - _subSrc.x ) );
    dY = abs( CSubHex::Diff( _subDest.y - _subSrc.y ) );

    int iStraight = abs( dX - dY );
    int iDiag     = __min( dX, dY );
    return ( ( 3 * iDiag + 2 * iStraight + 2 ) / 4 );
}

// since we can only move hex to hex, I go diaganol until all that
// is left is a straight run.
// return distance in CHexCoord's
int CGameMap::GetRangeDistance( CHexCoord& hex1, CHexCoord& hex2 )
{

    int x = abs( CHexCoord::Diff( hex1.X( ) - hex2.X( ) ) );
    int y = abs( CHexCoord::Diff( hex1.Y( ) - hex2.Y( ) ) );

    int iStraight = abs( x - y );
    int iDiag     = __min( x, y );
    return ( ( 3 * iDiag + 2 * iStraight + 1 ) / 2 );
}

// note - need to /4 to equal above
int CGameMap::GetRangeDistance( CSubHex& _sub1, CSubHex& _sub2 )
{

    int x = abs( CSubHex::Diff( _sub1.x - _sub2.x ) );
    int y = abs( CSubHex::Diff( _sub1.y - _sub2.y ) );

    int iStraight = abs( x - y );
    int iDiag     = __min( x, y );
    return ( 3 * iDiag + 2 * iStraight );
}

// iDir 0 == N, 1 == NE, 2 == E, ...
static int aiAddX[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
static int aiAddY[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
int        CGameMap::GetTerrainCost( CHexCoord const& hex, int iDir, int iWheel )
{

    return ( _GetTerrainCost( theMap._GetHex( hex ), theMap.GetHex( hex.X( ) + aiAddX[iDir], hex.Y( ) + aiAddY[iDir] ),
                              iDir, iWheel ) );
}

// returns the multiplier for entering this pHexDest from pHex
int CGameMap::GetTerrainCost( CHexCoord const& hex, CHexCoord const& hexNext, int iDir, int iWheel )
{

    CHex* pHex = theMap._GetHex( hex );

    CHex* pHexDest;
    if ( hex != hexNext )
        pHexDest = theMap._GetHex( hexNext );
    else
    {
        pHexDest = pHex;
        iDir     = 0;
    }

    return ( _GetTerrainCost( pHex, pHexDest, iDir, iWheel ) );
}

// returns the multiplier for entering this pHexDest from pHex
int CGameMap::GetTerrainCost( CHex const* pHex, CHex const* pHexDest, int iDir, int iWheel )
{

    return ( _GetTerrainCost( pHex, pHexDest, iDir, iWheel ) );
}

// returns the multiplier for entering this pHexDest from pHex
int CGameMap::_GetTerrainCost( CHex const* pHex, CHex const* pHexDest, int iDir, int iWheel )
{

    ASSERT( ( 0 <= iDir ) && ( iDir < 8 ) );
    ASSERT( ( 0 <= iWheel ) && ( iWheel < NUM_WHEEL_TYPES ) );

    int iTyp;
    if ( pHexDest->GetUnits( ) & CHex::bldg )
        iTyp = CHex::city;
    else if ( pHexDest->GetUnits( ) & CHex::bridge )
        iTyp = CHex::road;
    else
        iTyp = pHexDest->GetType( );

    // movement speed of this unit on this terrain
    int iRtn = theTerrain.GetData( iTyp ).GetWheelMult( iWheel );
    if ( iRtn == 0 )
    {
        // special case - boats can go in bldgs (but not bombed cities)
        //   (only buildings with ship exits)
        if ( ( iWheel == CWheelTypes::water ) && ( pHexDest->GetUnits( ) & CHex::bldg ) )
        {
            CBuilding* pBldg = theBuildingHex._GetBuilding( pHexDest->GetHex( ) );
            if ( ( pBldg != NULL ) && ( pBldg->GetData( )->HasShipExit( ) ) )
                return ( 1 );
        }
        return ( 0 );
    }

    // take diaganol into account
    if ( iDir & 1 )
        iRtn *= 3;
    else
        iRtn *= 2;

    // altitude change
    int iAlt = pHexDest->GetAlt( ) - pHex->GetAlt( );
    if ( iAlt >= 0 )
        iRtn += ( iRtn * iAlt ) / 8;
    else
    {
        iRtn -= ( iRtn * iAlt ) / 16;
        iRtn = __max( 1, iRtn );
    }

    return ( iRtn );
}


struct tagCHECK3
{
    int iAbove;
    int iBelow;
    int iNewAlt;
};

static int fnEnumAltCost( CHex* pHex, CHexCoord, void* piRtn )
{

    struct tagCHECK3* pck = (struct tagCHECK3*)piRtn;

    pck->iAbove += __max( 0, pck->iNewAlt - pHex->GetAlt( ) );
    pck->iBelow += __max( 0, pHex->GetAlt( ) - pck->iNewAlt );

    return ( FALSE );
}

int CGameMap::FoundationCost( CHexCoord const& hex, int iType, int iDir, CVehicle const* pVeh, int* piAlt,
                              int* piWhy ) const
{
    const int MAX_ALT_CHANGE = 10;

    // NOTE - pVeh does NOT need to be legit - it is -1 for NO vehicle allowed

    CStructureData const* pData = theStructures.GetData( iType );

    // we go in a giant enumeration where we get:
    // 1: 1 past all alt verticies for total range of altitude (so we don't create cliffs)
    // 2: all border hexes to see if buildings or water force our alt
    // 3: all hexes we want to see if:
    //    a: any buildings are there
    //    b: if the water/land mix is correct
    //    c: (optional) if any vehicles are in the way
    // 4: get the average altitude
    BOOL bBldg    = FALSE;  // building in way
    BOOL bVeh     = FALSE;  // vehicle in way
    BOOL bWater   = FALSE;  // water on location
    BOOL bLand    = FALSE;  // land on location
    int  iCost    = 0;      // cost to build
    int  iAlt     = 0;      // avg altitude of location
    int  iBldgAlt = -1;     // forced altitude because of buildings/water
    int  iMinAlt  = 128;    // minimum altutude of area
    int  iMaxAlt  = 0;      // maximum altitidue of area

    CHexCoord _hex( hex.X( ) - 2, hex.Y( ) - 2 );
    _hex.Wrap( );
    int xSize = iDir & 1 ? pData->GetCY( ) : pData->GetCX( );
    int xMax  = xSize + 4;
    int ySize = iDir & 1 ? pData->GetCX( ) : pData->GetCY( );
    int yMax  = ySize + 4;

    for ( int y = 0; y < yMax; y++, _hex.Yinc( ) )
    {
        for ( int x = 0; x < xMax; x++, _hex.Xinc( ) )
        {
            CHex const* pHex   = _GetHex( _hex );
            int         iAltOn = pHex->GetAlt( );
            if ( iAltOn < CHex::sea_level )
                iAltOn = CHex::sea_level;

            // altitude range
            // for speed we test for min/max too great here
            if ( iMinAlt > iAltOn )
            {
                iMinAlt = iAltOn;
                if ( iMaxAlt - iMinAlt > MAX_ALT_CHANGE * 2 )
                {
                    if ( piWhy )
                        *piWhy = steep;
                    return ( -1 );
                }
            }
            if ( iMaxAlt < iAltOn )
            {
                iMaxAlt = iAltOn;
                if ( iMaxAlt - iMinAlt > MAX_ALT_CHANGE * 2 )
                {
                    if ( piWhy )
                        *piWhy = steep;
                    return ( -1 );
                }
            }

            // item (1-above) only on outer edge
            if ( ( x == 0 ) || ( y == 0 ) || ( x == xMax - 1 ) || ( y == yMax - 1 ) )
                continue;

            // we are now either in or directly adjacent to the location

            // building is false if it's dead or dying
            BYTE bFlags = pHex->GetUnits( );
            if ( bFlags & CHex::bldg )
            {
                CBuilding* pBldg = theBuildingHex._GetBuilding( _hex );
                if ( pBldg == NULL )
                {
                    TRAP( );
                    bFlags &= ~CHex::bldg;
                }
                else if ( pBldg->IsFlag( CUnit::dead ) || pBldg->IsFlag( CUnit::dying ) )
                    bFlags &= ~CHex::bldg;
            }

            // is there a building or water adjacent (we assume not on - test for that lower)
            // this is faster than testing to see if on border, etc.
            if ( ( ( bFlags & ( CHex::bldg | CHex::bridge ) ) != 0 ) && ( pVeh || ( pHex->GetVisible( ) ) ) )
            {
                if ( iBldgAlt == -1 )
                    iBldgAlt = iAltOn;
                else
                    // fast check
                    if ( abs( iBldgAlt - iAltOn ) > 10 )
                    {
                        if ( piWhy )
                            *piWhy = bldg_next;
                        return ( -1 );
                    }
            }
            // if its ocean/lake we have the same thing
            else if ( ( pHex->GetType( ) == CHex::lake ) || ( pHex->GetType( ) == CHex::ocean ) ||
                      ( pHex->GetType( ) == CHex::coastline ) )
            {
                if ( iBldgAlt == -1 )
                    iBldgAlt = CHex::sea_level;
                else if ( abs( iBldgAlt - CHex::sea_level ) > 6 )
                {
                    if ( piWhy )
                        *piWhy = water_next;
                    return ( -1 );
                }
            }

            // item (2-above) all bordering edges
            // need to keep top/right borders for next step
            if ( ( x == 1 ) || ( y == yMax - 2 ) )
                continue;

            // average alt of all vertices (each hex is LL corner altitude)
            iAlt += iAltOn;

            // now get rid of rest of border hexes
            if ( ( y == 1 ) || ( x == xMax - 2 ) )
                continue;

            // if there is a building or river already here forget it
            if ( ( pHex->GetType( ) == CHex::river ) ||
                 ( ( ( bFlags & ( CHex::bldg | CHex::bridge ) ) != 0 ) && ( pVeh || ( pHex->GetVisible( ) ) ) ) )
            {
                if ( piWhy )
                    *piWhy = bldg_or_river;
                return ( -1 );
            }

            // if we are testing for vehicles make sure its ours
            if ( ( pVeh != NULL ) && ( bFlags & ( CHex::ul | CHex::ur | CHex::ll | CHex::lr ) ) )
            {
                CSubHex _sub( _hex.X( ) * 2, _hex.Y( ) * 2 );
                if ( ( bFlags & CHex::ul ) && ( theVehicleHex._GetVehicle( _sub ) != pVeh ) )
                {
                    if ( piWhy )
                        *piWhy = veh_in_way;
                    return ( -1 );
                }
                _sub.x++;
                if ( ( bFlags & CHex::ur ) && ( theVehicleHex._GetVehicle( _sub ) != pVeh ) )
                {
                    if ( piWhy )
                        *piWhy = veh_in_way;
                    return ( -1 );
                }
                _sub.y++;
                if ( ( bFlags & CHex::lr ) && ( theVehicleHex._GetVehicle( _sub ) != pVeh ) )
                {
                    if ( piWhy )
                        *piWhy = veh_in_way;
                    return ( -1 );
                }
                _sub.x--;
                if ( ( bFlags & CHex::ll ) && ( theVehicleHex._GetVehicle( _sub ) != pVeh ) )
                {
                    if ( piWhy )
                        *piWhy = veh_in_way;
                    return ( -1 );
                }
            }

            // see if water (or coastline)
            if ( ( pHex->IsWater( ) || ( pHex->GetType( ) == CHex::coastline ) ) && ( !pData->IsPartWater( ) ) )
            {
                if ( piWhy )
                    *piWhy = water;
                return ( -1 );
            }
            if ( pHex->IsWater( ) )
                bWater = TRUE;
            else
                bLand = TRUE;

            // add in the cost
            iCost += theTerrain.GetData( pHex->GetType( ) ).GetBuildMult( );
        }
        _hex.X( hex.X( ) - 2 );
        _hex.Wrap( );
    }

    // we know we are all land if not part water (test above). But if part
    // water we don't know if we have either
    if ( ( pData->IsPartWater( ) ) && ( ( !bWater ) || ( !bLand ) ) )
    {
        if ( piWhy )
            *piWhy = no_water;
        return ( -1 );
    }

    // get the altitude of all hex verticies
    iAlt = ( ( iAlt / ( ( xSize + 1 ) * ( ySize + 1 ) ) ) + 2 ) & ~0x03;
    ASSERT( ( ( iMinAlt & ~0x03 ) <= iAlt ) && ( iAlt <= ( ( iMaxAlt + 2 ) & ~0x03 ) ) );

    // if there is a bldg alt we must be the same
    if ( ( iBldgAlt != -1 ) && ( iAlt != iBldgAlt ) )
    {
        // we will move up to 4 to match
        if ( abs( iAlt - iBldgAlt ) <= 4 )
            iAlt = iBldgAlt;
        else
        {
            if ( piWhy )
                *piWhy = bldg_next;
            return ( -1 );
        }
    }

    // we must be withing 8 of the min & max
    if ( ( abs( iAlt - iMinAlt ) > MAX_ALT_CHANGE ) || ( abs( iAlt - iMaxAlt ) > MAX_ALT_CHANGE ) )
    {
        if ( piWhy )
            *piWhy = steep;
        return ( -1 );
    }

    // make sure the exit(s) can exit to land/water (we don't worry about terrain type)
    if ( !CStructureData::CanBuild( hex, iDir, iType, FALSE, FALSE ) )
    {
        if ( piWhy )
            *piWhy = no_land_exit;
        return ( -1 );
    }
    if ( !CStructureData::CanBuild( hex, iDir, iType, TRUE, FALSE ) )
    {
        if ( piWhy )
            *piWhy = no_water_exit;
        return ( -1 );
    }

    // determine the cost of bringing the land to this level
    struct tagCHECK3 ck3;
    ck3.iAbove = ck3.iBelow = 0;
    ck3.iNewAlt             = iAlt;
    theMap.EnumHexes( hex, xSize, ySize, fnEnumAltCost, &ck3 );

    if ( piAlt != NULL )
        *piAlt = iAlt;

    if ( piWhy )
        *piWhy = ok;
    return ( iCost + ( ck3.iAbove + __max( 0, ck3.iBelow - ck3.iAbove ) ) );
}

//---------------------------- C T i l e D r a w ---------------------------
//
// The display list

class CTileDraw
{
  public:
    CTileDraw( ): m_btreetile1( FALSE ), m_btreetile2( FALSE ), m_btreetile3( FALSE )
    {
        m_apbtreetile[0] = &m_btreetile1;
        m_apbtreetile[1] = &m_btreetile2;
        m_apbtreetile[2] = &m_btreetile3;
    }

    void AddTile( CTileDrawInfo* );
    void DrawRow( );
    void Flush( );

  private:
    BTree<CTileDrawInfo>  m_btreetile1;
    BTree<CTileDrawInfo>  m_btreetile2;
    BTree<CTileDrawInfo>  m_btreetile3;
    BTree<CTileDrawInfo>* m_apbtreetile[3];
};

//--------------------------------------------------------------------------
// CTileDraw::AddTile
//--------------------------------------------------------------------------
void CTileDraw::AddTile( CTileDrawInfo* pdrawinfo )
{
    ASSERT_STRICT( pdrawinfo );

    // If the unit is in one of the previous rows, delete it from there
    // Check two previous rows because vehicles can skip a row

    for ( int i = 0; i < 2; ++i )
    {
        BTreeIter<CTileDrawInfo> iter( m_apbtreetile[i], m_apbtreetile[i]->Find( pdrawinfo ) );

        if ( iter.Value( ) )
            iter.Delete( );
    }

    //	for ( int i = 0; i < 2; ++i )
    //		for ( BTreeIter<CTileDrawInfo> iter( m_apbtreetile[i] );
    //				iter.Value();
    //				iter.ToNext() )
    //		{
    //			ASSERT_STRICT( iter.Value() );
    //
    //			if ( *iter.Value() == *pdrawinfo )
    //			{
    //				iter.Delete();
    //				break;
    //			}
    //		}

    // Add the tile to the current row

    m_apbtreetile[2]->Insert( pdrawinfo );
}

//-------------------------------------------------------------------------------------------------
//	CTileDraw::Flush
//-------------------------------------------------------------------------------------------------
void CTileDraw::Flush( )
{
    while ( m_apbtreetile[0]->Count( ) || m_apbtreetile[1]->Count( ) || m_apbtreetile[2]->Count( ) ) DrawRow( );
}

//-------------------------------------------------------------------------------------------------
//	CTileDraw::DrawRow
//-------------------------------------------------------------------------------------------------
void CTileDraw::DrawRow( )
{
    // Draw the oldest row

    for ( BTreeIter<CTileDrawInfo> iter( m_apbtreetile[0] ); iter.Value( ); iter.ToNext( ) )
    {
        ASSERT_STRICT( iter.Value( ) );

        CTileDrawInfo* pdrawinfo = iter.Value( );

        pdrawinfo->Draw( );
    }

    BTree<CTileDrawInfo>* pbtreetileTemp = m_apbtreetile[0];

    m_apbtreetile[0] = m_apbtreetile[1];
    m_apbtreetile[1] = m_apbtreetile[2];
    m_apbtreetile[2] = pbtreetileTemp;

    ASSERT_STRICT( m_apbtreetile[2] );

    m_apbtreetile[2]->Purge( );
}

//---------------------------------------------------------------------------
// CGameMap::Update
//---------------------------------------------------------------------------
void CGameMap::Update( CAnimAtr& aa )
{
    ASSERT_STRICT_VALID( &aa );
    ASSERT_STRICT( m_pHex != NULL );
    ASSERT_STRICT_VALID( this );

    // if we GPF drawing we just keep going
    try
    {
        UpdateRect( aa, aa.m_dibwnd.GetRect( ), CDrawParms::invalidate );

        if ( bForceDraw )
            aa.m_dibwnd.Invalidate( NULL );
    }

    // and we move to try and fix it
    catch ( ... )
    {
        aa.MoveCenterPixels( 1, 1 );
    }
}

//---------------------------------------------------------------------------
// CGameMap::UpdateRect
//
// GGFIXIT: Need to adjust unit maploc centers when crossing map edges?
//---------------------------------------------------------------------------
void CGameMap::UpdateRect( CAnimAtr& aa, CRect rect, CDrawParms::UPDATE_MODE eMode )
{

    try
    {

        CRect rectDIB = aa.m_dibwnd.GetRect( );

        rect &= rectDIB;

        if ( rect.IsRectEmpty( ) )
            return;

        BOOL bDraw = CDrawParms::draw == eMode;

        aa.SetCurrent( );  // Update window-drawing globals

        CDrawParms::SetUpdateFlags( eMode );
        CDrawParms::SetClipRect( rect );

        CHexCoord hexTL( aa._WindowToHex( CPoint( rect.left, rect.top ) ) );
        CHexCoord hexTR( aa._WindowToHex( CPoint( rect.right - 1, rect.top ) ) );

        CViewHexCoord ptTL( hexTL );
        CViewHexCoord ptTR( hexTR );

        CTileDraw tiledraw;

        int iTopY   = min( ptTL.y, ptTR.y ) - 1;
        int iLeftX  = ptTL.x - 1;
        int iRightX = ptTR.x + 1;

        CVehicle* pvehicle;
        BOOL      bExtraRows = FALSE;
        int       iRowTopY   = 0;
        CRect     rectHexBound;

        int           iMaxBuildingHeight = theStructures.GetTallestBuildingHeight( aa.m_iZoom );
        CDrawInfoPool drawinfopool;

        for ( int y = iTopY;; ++y )
        {
            // Draw rows until all the terrain in view is drawn
            // Then draw n more rows to get the tops of buildings that start below the window

            if ( iRowTopY >= rect.bottom )
                bExtraRows = TRUE;

            if ( bExtraRows && iRowTopY - rect.bottom > iMaxBuildingHeight )
                break;

            iRowTopY = INT_MAX;

            for ( int x = iLeftX; x <= iRightX; ++x )
            {
                CHexCoord hexcoord( CViewHexCoord( x, y ), TRUE );
                CHexCoord hexcoordWrapped = hexcoord;
                CHex*     phex            = theMap.GetHex( hexcoord );

                hexcoordWrapped.Wrap( );

                ASSERT_STRICT( phex );

                CSubHex subhex( hexcoordWrapped );

                BYTE byUnits   = phex->GetUnits( );
                BOOL bBuilding = byUnits & CHex::bldg;

                // Handle building

                if ( bBuilding )
                {
                    CBuilding* pbuilding = theBuildingHex._GetBuilding( hexcoordWrapped );

                    ASSERT_STRICT_VALID( pbuilding );

                    if ( !pbuilding->IsVisible( ) )
                    {
                        rectHexBound = phex->Draw( hexcoord );

                        iRowTopY = min( iRowTopY, rectHexBound.top );
                    }
                    else
                    {
                        aa.CalcWindowHexBound( hexcoord, rectHexBound );

                        iRowTopY = min( iRowTopY, rectHexBound.top );

                        pbuilding->DrawFoundation( hexcoord );

                        CHexCoord hexBuilding = pbuilding->GetLeftHex( xiDir );

                        hexBuilding.ToNearestHex( hexcoord );

                        if ( x == iLeftX || y == iTopY || hexBuilding == hexcoord )
                        {
                            if ( pbuilding->IsTwoPiece( ) )
                                if ( bDraw )
                                    tiledraw.AddTile( drawinfopool.GetStructureDrawInfo(
                                        pbuilding, CTileDrawInfo::building, hexBuilding, pbuilding->GetMapLoc( ),
                                        CStructureSprite::BACKGROUND_LAYER ) );
                                else
                                    pbuilding->DrawLayer( hexBuilding, CStructureSprite::BACKGROUND_LAYER );

                            if ( bDraw )
                                tiledraw.AddTile( drawinfopool.GetStructureDrawInfo(
                                    pbuilding, CTileDrawInfo::building, hexBuilding, pbuilding->GetMapLoc( ),
                                    CStructureSprite::FOREGROUND_LAYER ) );
                            else
                                pbuilding->DrawLayer( hexBuilding, CStructureSprite::FOREGROUND_LAYER );
                        }
                    }

                    continue;
                }

                if ( !bExtraRows )
                {
                    rectHexBound = phex->Draw( hexcoord );

                    iRowTopY = min( iRowTopY, rectHexBound.top );
                }
                else
                {
                    aa.CalcWindowHexBound( hexcoord, rectHexBound );

                    iRowTopY = min( iRowTopY, rectHexBound.top );
                }

                // Handle bridge

                if ( byUnits & CHex::bridge )
                {
                    CMapLoc maploc( hexcoord );

                    maploc.x += 32;
                    maploc.y += 32;

                    CBridgeUnit* pbridge = theBridgeHex.GetBridge( hexcoordWrapped );

                    ASSERT_VALID( pbridge );

                    if ( pbridge->IsTwoPiece( ) )
                        if ( bDraw )
                            tiledraw.AddTile( drawinfopool.GetStructureDrawInfo( pbridge, CTileDrawInfo::bridge,
                                                                                 hexcoord, maploc,
                                                                                 CStructureSprite::BACKGROUND_LAYER ) );
                        else
                            pbridge->DrawLayer( hexcoord, CStructureSprite::BACKGROUND_LAYER );

                    if ( bDraw )
                        tiledraw.AddTile( drawinfopool.GetStructureDrawInfo(
                            pbridge, CTileDrawInfo::bridge, hexcoord, maploc, CStructureSprite::FOREGROUND_LAYER ) );
                    else
                        pbridge->DrawLayer( hexcoord, CStructureSprite::FOREGROUND_LAYER );
                }

                if ( bExtraRows )  // Just looking for structures
                    continue;

                // Handle vehicles

                if ( phex->GetVisibility( ) && !bBuilding )
                {
                    if ( byUnits & CHex::veh )
                    {
                        if ( byUnits & CHex::ul )
                        {
                            pvehicle = theVehicleHex.GetVehicle( subhex );

                            if ( pvehicle && pvehicle->IsVisible( ) && !pvehicle->IsInBuilding( ) )
                                if ( bDraw )
                                    tiledraw.AddTile( drawinfopool.GetVehicleDrawInfo( pvehicle, hexcoord ) );
                                else
                                    pvehicle->Draw( hexcoord );
                        }

                        subhex.x++;

                        if ( byUnits & CHex::ur )
                        {
                            pvehicle = theVehicleHex.GetVehicle( subhex );

                            if ( pvehicle && pvehicle->IsVisible( ) && !pvehicle->IsInBuilding( ) )
                                if ( bDraw )
                                    tiledraw.AddTile( drawinfopool.GetVehicleDrawInfo( pvehicle, hexcoord ) );
                                else
                                    pvehicle->Draw( hexcoord );
                        }

                        subhex.y++;

                        if ( byUnits & CHex::lr )
                        {
                            pvehicle = theVehicleHex.GetVehicle( subhex );

                            if ( pvehicle && pvehicle->IsVisible( ) && !pvehicle->IsInBuilding( ) )
                                if ( bDraw )
                                    tiledraw.AddTile( drawinfopool.GetVehicleDrawInfo( pvehicle, hexcoord ) );
                                else
                                    pvehicle->Draw( hexcoord );
                        }

                        subhex.x--;

                        if ( byUnits & CHex::ll )
                        {
                            pvehicle = theVehicleHex.GetVehicle( subhex );

                            if ( pvehicle && pvehicle->IsVisible( ) && !pvehicle->IsInBuilding( ) )
                                if ( bDraw )
                                    tiledraw.AddTile( drawinfopool.GetVehicleDrawInfo( pvehicle, hexcoord ) );
                                else
                                    pvehicle->Draw( hexcoord );
                        }
                    }
                }

                // Handle trees

                if ( CHex::forest == phex->GetType( ) )
                {
                    CMapLoc maplocCenter( hexcoord );

                    maplocCenter.x += MAX_HEX_HT >> 1;
                    maplocCenter.y += MAX_HEX_HT >> 1;

                    CMapLoc maplocRear( maplocCenter );
                    CMapLoc maplocFront( maplocCenter );

                    int iOffset = 3 * MAX_HEX_HT >> 3;

                    switch ( xiDir )
                    {
                    case 0:
                        maplocRear.x += iOffset;
                        maplocRear.y -= iOffset;
                        maplocFront.x -= iOffset;
                        maplocFront.y += iOffset;
                        break;

                    case 1:
                        maplocRear.x += iOffset;
                        maplocRear.y += iOffset;
                        maplocFront.x -= iOffset;
                        maplocFront.y -= iOffset;
                        break;

                    case 2:
                        maplocRear.x -= iOffset;
                        maplocRear.y += iOffset;
                        maplocFront.x += iOffset;
                        maplocFront.y -= iOffset;
                        break;

                    case 3:
                        maplocRear.x -= iOffset;
                        maplocRear.y -= iOffset;
                        maplocFront.x += iOffset;
                        maplocFront.y += iOffset;
                        break;

                    default:
                        ASSERT( 0 );
                    }

                    CTree* ptree = theEffects.GetTree( phex->GetTree( ) );

                    // FIXIT: If hex backward-facing draw immediately?

                    if ( bDraw )
                    {
                        tiledraw.AddTile( drawinfopool.GetStructureDrawInfo(
                            ptree, CTileDrawInfo::tree, hexcoord, maplocRear, CStructureSprite::BACKGROUND_LAYER ) );

                        tiledraw.AddTile( drawinfopool.GetStructureDrawInfo(
                            ptree, CTileDrawInfo::tree, hexcoord, maplocFront, CStructureSprite::FOREGROUND_LAYER ) );
                    }
                    else
                    {
                        ptree->DrawLayer( hexcoord, CStructureSprite::BACKGROUND_LAYER );
                        ptree->DrawLayer( hexcoord, CStructureSprite::FOREGROUND_LAYER );
                    }
                }

                // Everything else is invisible, if the hex is invisible

                if ( !phex->GetVisibility( ) )
                    continue;

                // Handle projectiles/explosions

                if ( byUnits & CHex::proj )
                {
                    CProjBase* pprojbase = theProjMap.GetFirst( hexcoordWrapped );

                    while ( pprojbase != NULL )
                    {
                        //					ASSERT_STRICT_VALID( pprojlist );

                        // BUGBUG					for (	POSITION	position = pprojlist->GetHeadPosition(); NULL != position;
                        // )
                        //					{
                        //						CProjBase	*pprojbase = pprojlist->GetNext( position );

                        // BUGBUG						if ( pprojbase )
                        //						{
                        ASSERT_STRICT_VALID( pprojbase );

                        switch ( pprojbase->GetType( ) )
                        {
                        case CProjBase::explosion:

                            if ( bDraw )
                                tiledraw.AddTile( drawinfopool.GetExplosionDrawInfo( pprojbase, hexcoord ) );
                            else
                                pprojbase->Draw( hexcoord );

                            break;

                        case CProjBase::projectile:

                            if ( bDraw )
                                tiledraw.AddTile( drawinfopool.GetTileDrawInfo( pprojbase, CTileDrawInfo::projectile,
                                                                                hexcoord, pprojbase->GetMapLoc( ) ) );
                            else
                                pprojbase->Draw( hexcoord );

                            break;

                        default:
                            ASSERT( 0 );
                        }
                        pprojbase = theProjMap.GetNext( pprojbase );
                    }
                    // bugbug					}
                    //				}
                }
            }

            if ( bDraw )
                tiledraw.DrawRow( );
        }

        if ( bDraw )
            tiledraw.Flush( );
    }  // end try

    catch ( ... )
    {
        aa.MoveCenterPixels( 1, 1 );
    }
}

static int fnEnumCurOn( CHex* pHex, CHexCoord, void* )
{

    pHex->SetCursor( );
    return ( FALSE );
}

static int fnEnumCurOff( CHex* pHex, CHexCoord _hex, void* )
{

    _hex.SetInvalidated( );
    pHex->ClrCursor( );
    return ( FALSE );
}

static CHexCoord GetCurExit( int iBldgDir, int iBldg, int iExitDir, BOOL bShip )
{

    CHexCoord _hex;

    CBuilding::DetermineExit( iBldgDir, iBldg, _hex, iExitDir, bShip );

    switch ( iExitDir )
    {
    case 0:
        _hex.Ydec( );
        break;
    case 1:
        _hex.Xinc( );
        break;
    case 2:
        _hex.Yinc( );
        break;
    case 3:
        _hex.Xdec( );
        break;
    }

    _hex.Wrap( );
    return ( _hex );
}

void CGameMap::SetBldgCur( CHexCoord const& hex, int iBldg, int iBldgDir, int iTyp )
{

    ClrBldgCur( );

    switch ( iTyp )
    {
    case 0:
        m_iBldgCur = CHex::ok_cur;
        break;
    case 2:
        m_iBldgCur = CHex::lousy_cur;
        break;
    default:
        m_iBldgCur = CHex::bad_cur;
        break;
    }
    m_hexBldgCur = hex;

    CStructureData const* pData = theStructures.GetData( iBldg );
    m_cxBldgCur                 = iBldgDir & 1 ? pData->GetCY( ) : pData->GetCX( );
    m_cyBldgCur                 = iBldgDir & 1 ? pData->GetCX( ) : pData->GetCY( );
    m_iLandDir                  = ( pData->GetExitDir( ) - iBldgDir ) & 3;
    m_iShipDir                  = ( pData->GetShipDir( ) - iBldgDir ) & 3;

    theMap.EnumHexes( hex, m_cxBldgCur, m_cyBldgCur, fnEnumCurOn, NULL );

    // set the exit hexes
    //   always land
    CHexCoord _hex = hex + GetCurExit( iBldgDir, iBldg, m_iLandDir, FALSE );
    _hex.Wrap( );
    m_pLandExit = _GetHex( _hex );
    m_pLandExit->SetCursor( );

    if ( pData->HasShipExit( ) )
    {
        CHexCoord _hex = hex + GetCurExit( iBldgDir, iBldg, m_iShipDir, TRUE );
        _hex.Wrap( );
        m_pShipExit = _GetHex( _hex );
        m_pShipExit->SetCursor( );
    }
    else
        m_pShipExit = NULL;
}

void CGameMap::ClrBldgCur( )
{

    theMap.EnumHexes( m_hexBldgCur, m_cxBldgCur, m_cyBldgCur, fnEnumCurOff, NULL );
    if ( m_pLandExit != NULL )
    {
        m_pLandExit->ClrCursor( );
        m_pLandExit->GetHex( ).SetInvalidated( );
    }
    if ( m_pShipExit != NULL )
    {
        m_pShipExit->ClrCursor( );
        m_pShipExit->GetHex( ).SetInvalidated( );
    }

    m_pLandExit = m_pShipExit = NULL;
    m_cxBldgCur = m_cyBldgCur = 0;
}

void CHex::Serialize( CArchive& ar )
{

    CSimpleTile::Serialize( ar );

    if ( ar.IsStoring( ) )
    {
        ar << m_bType << m_bAlt << m_bUnit;

        ar << (WORD)m_psprite->GetID( );
        ar << (WORD)m_psprite->GetIndex( );
    }
    else
    {
        ar >> m_bType >> m_bAlt >> m_bUnit;
        WORD iID;
        ar >> iID;
        WORD iIndex;
        ar >> iIndex;
        m_psprite = theTerrain.GetSprite( iID, iIndex );
    }
}

void CGameMap::Serialize( CArchive& ar )
{

    CObject::Serialize( ar );

    if ( ar.IsStoring( ) )
    {
        ar << m_eX << m_eY << m_iSideSize << m_iSideShift << m_iHexMask << m_iWidthHalf << m_iSubMask;
        ar << m_iLocMask << m_iLocHalf;

        long  lTotal = (long)m_eX * (long)m_eY;
        CHex* pHex   = m_pHex;
        while ( lTotal-- )
        {
            pHex->Serialize( ar );
            pHex++;
        }
    }

    else
    {
        ar >> m_eX >> m_eY >> m_iSideSize >> m_iSideShift >> m_iHexMask >> m_iWidthHalf >> m_iSubMask;
        ar >> m_iLocMask >> m_iLocHalf;

        long lTotal = (long)m_eX * (long)m_eY;
        m_pHex      = new CHex[lTotal];
        CHex* pHex  = m_pHex;
        while ( lTotal-- )
        {
            pHex->Serialize( ar );
            pHex++;
        }

        m_ptrhexvalidmatrix = new CHexValidMatrix( m_iSideShift - 1, m_iSideShift - 1 );
    }
}

#ifdef _DEBUG
void CGameMap::AssertValid( ) const
{

    // assert base object
    CObject::AssertValid( );

    ASSERT( this == &theMap );
    if ( m_pHex == NULL )
        return;

    long lTotal = (long)m_eX * (long)m_eY;
    ASSERT( AfxIsValidAddress( m_pHex, sizeof( CHex ) * lTotal ) );
}
#endif
