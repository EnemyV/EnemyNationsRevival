#ifndef __TERRAIN_INL__
#define __TERRAIN_INL__

#include "terrain.h"
#include "base.h"
#include "building.h"

#ifndef __BASE_H__
#warning "base.h must be included before including this .inl file!"
#endif

// CHexCoord
// FIXMEFIXME
inline CHexCoord::CHexCoord( CSubHex const& sub ) {
    m_iX = sub.x >> 1; m_iY = sub.y >> 1;
}

inline CArchive& operator<< ( CArchive& ar, const CHexCoord hex ) {
    ar << (WORD)hex.m_iX; ar << (WORD)hex.m_iY;
    return ar;
}
inline CArchive& operator>> ( CArchive& ar, CHexCoord& hex ) {
    WORD w; ar >> w; hex.m_iX = w; ar >> w; hex.m_iY = w;
    return ar;
}

inline void CHexCoord::X( int _x ) {
    m_iX = _x & theMap.m_iHexMask;
}
inline void CHexCoord::Y( int _y ) {
    m_iY = _y & theMap.m_iHexMask;
}
inline CHexCoord::CHexCoord( CMapLoc const& pt ) {
    m_iX = pt.x >> HEX_HT_PWR;
    m_iY = pt.y >> HEX_HT_PWR;
}
inline CHexCoord::CHexCoord(unsigned long val)
{
    m_iX = (int)((val >> 16) & 0xFFFF);
    m_iY = (int)(val & 0xFFFF);
}
inline void CHexCoord::Xinc() { m_iX++; m_iX &= theMap.m_iHexMask; }
inline void CHexCoord::Yinc() { m_iY++; m_iY &= theMap.m_iHexMask; }
inline void CHexCoord::Xdec() { m_iX--; m_iX &= theMap.m_iHexMask; }
inline void CHexCoord::Ydec() { m_iY--; m_iY &= theMap.m_iHexMask; }

// same here, something up! // FIXMEFIXME
// actually, i dont think there's a issue, false error?
// in debug, i saw no issues
inline BOOL CHexCoord::SameHex( CSubHex src ) const {
    return ( ( m_iX == src.x >> 1 ) && ( m_iY == src.y >> 1 ) );
}
inline void	CHexCoord::SetInvalidated() { theMap.GetHexValidMatrix()->SetInvalidated( m_iX, m_iY ); }
inline BOOL	CHexCoord::IsInvalidated() const { return theMap.GetHexValidMatrix()->IsInvalidated( m_iX, m_iY ); }
inline void CHexCoord::ClearInvalidated() { theMap.GetHexValidMatrix()->Clear(); }

// what's wrong here?  // FIXMEFIXME
inline CSubHex::CSubHex( const CHexCoord& hex ) {
    x = hex.X() << 1; y = hex.Y() << 1; 
}

inline CSubHex::CSubHex( CMapLoc const& pt ) {
    x = pt.x >> HEX_HT_PWR - 1;
    y = pt.y >> HEX_HT_PWR - 1;
}

inline CHexCoord& CHexCoord::Wrap() {
    m_iX &= theMap.m_iHexMask;
    m_iY &= theMap.m_iHexMask;
    return ( *this );
}
inline CHexCoord& CHexCoord::WrapX() {
    m_iX &= theMap.m_iHexMask;
    return ( *this );
}
inline CHexCoord& CHexCoord::WrapY() {
    m_iY &= theMap.m_iHexMask;
    return ( *this );
}

inline int CHexCoord::Wrap( int iVal ) {
    return ( iVal & theMap.m_iHexMask );
}

inline int CHexCoord::Diff( int iVal ) {
    iVal += theMap.m_iWidthHalf;
    iVal &= theMap.m_iHexMask;
    iVal -= theMap.m_iWidthHalf;
    return ( iVal );
}

inline CArchive& operator<< ( CArchive& ar, const CSubHex sub ) {
    ar << (WORD)sub.x; ar << (WORD)sub.y;
    return ar;
}
inline CArchive& operator>> ( CArchive& ar, CSubHex& sub ) {
    WORD w; ar >> w; sub.x = w; ar >> w; sub.y = w;
    return ar;
}

inline CSubHex& CSubHex::Wrap() {
    x &= theMap.m_iSubMask;
    y &= theMap.m_iSubMask;
    return ( *this );
}
inline CSubHex& CSubHex::WrapX() {
    x &= theMap.m_iSubMask;
    return ( *this );
}
inline CSubHex& CSubHex::WrapY() {
    y &= theMap.m_iSubMask;
    return ( *this );
}

inline int CSubHex::Wrap( int iVal ) {
    return ( iVal & theMap.m_iSubMask );
}

inline int CSubHex::Diff( int iVal ) {
    iVal += theMap.m_eX;
    iVal &= theMap.m_iSubMask;
    iVal -= theMap.m_eX;
    return ( iVal );
}

inline int CMapLoc::Wrap( int iVal ) {
    return ( iVal & theMap.m_iLocMask );
}

inline int CMapLoc::Diff( int iVal ) {
    iVal += theMap.m_iLocHalf;
    iVal &= theMap.m_iLocMask;
    iVal -= theMap.m_iLocHalf;
    return ( iVal );
}
inline CArchive& operator<< ( CArchive& ar, const CMapLoc loc ) {
    ar.Write( &loc, sizeof( CMapLoc ) );
    return ar;
}
inline CArchive& operator>> ( CArchive& ar, CMapLoc& loc ) {
    ar.Read( &loc, sizeof( CMapLoc ) );
    return ar;
}

// CMapLoc::CMapLoc - 
inline CMapLoc::CMapLoc( const CHexCoord& rhexcoord ) {
    x = rhexcoord.X() << HEX_HT_PWR;
    y = rhexcoord.Y() << HEX_HT_PWR;
}

inline CMapLoc::CMapLoc( const CSubHex& rsubhex ) {
    x = rsubhex.x << ( HEX_HT_PWR - 1 );
    y = rsubhex.y << ( HEX_HT_PWR - 1 );
}

inline CMapLoc& CMapLoc::Wrap() {
    x &= theMap.m_iLocMask;
    y &= theMap.m_iLocMask;
    return( *this );
}

//   CHex
//---------------------------------------------------------------------------
// CHex::GetCursorMode
//---------------------------------------------------------------------------
inline int CHex::GetCursorMode() const {

    // if no cursor - vamoose
    if ( ( !theMap.HaveBldgCur() ) || ( ( m_bAlt & 0x80 ) == 0 ) )
        return no_cur;

    // exit hexes
    if ( this == theMap.m_pLandExit )
        return ( IsWater() ? bad_cur : land_exit_cur );
    if ( this == theMap.m_pShipExit )
        return ( IsWater() ? bad_cur : sea_exit_cur );

    // only units we can see (cheat - if the hex is visible we can see it)
    if ( ( GetUnits() & veh ) && ( GetVisibility() ) )
        return warn_cur;

    return ( theMap.IsBldgCurOk() );
}

inline void CHex::AdjustY( int& y ) const {
    ASSERT_STRICT_VALID( this );
    y -= ( GetAltDraw() - sea_level ) << ( TERRAIN_HT_SHIFT - xiZoom );
}

inline int	CHex::GetAdjustY( int iZoom ) const {
    ASSERT_STRICT_VALID( this );
    return ( GetAltDraw() - sea_level ) << ( TERRAIN_HT_SHIFT - iZoom );
}

inline int CHex::GetAdjustStep() const {
    ASSERT_STRICT_VALID( this ); if ( GetAlt() < sea_level + map_step ) return ( 0 );
    return ( ( GetAltDraw() - sea_level ) / map_step );
}
inline BOOL CHex::IsWater() const { return ( ( GetType() == river ) || ( GetType() == ocean ) || ( GetType() == lake ) ); }
inline BOOL CHex::CanRoad() const { return ( ( GetType() != coastline ) && ( !IsWater() ) ); }

inline int	CHex::GetAltDraw() const {
    ASSERT_STRICT_VALID( this );
    return max( GetAlt(), sea_level );
}

inline void CHex::IncVisible() { m_bVisible++; }
inline void CHex::IncVisible( int iNum ) { m_bVisible += (BYTE)iNum; }
inline void CHex::DecVisible() { m_bVisible--; }
inline int CHex::GetVisible() const { return ( int( m_bVisible ) ); }
inline BOOL CHex::GetVisibility() const { return ( BOOL( m_bVisible ) ); }

inline CHexCoord CHex::GetHex() const {
    int iNum = this - theMap.m_pHex;
    return CHexCoord( iNum & theMap.m_iHexMask, iNum >> theMap.m_iSideShift );
}

// note - all of these may not be handled right by the VC optimizer
inline int CHex::GetAlt() const {
    ASSERT_STRICT_VALID( this );
    return ( int( m_bAlt ) & 0x7F );
}
inline int CHex::GetSeaAlt() const {
    ASSERT_STRICT_VALID( this );
    return ( __max( sea_level, int( m_bAlt ) ) & 0x7F );
}

inline void CHex::SetAlt( int iAlt ) {
    ASSERT( ( 0 <= iAlt ) && ( iAlt <= 127 ) );
    iAlt = __minmax( 0, 127, iAlt );
    m_bAlt = (BYTE)( iAlt | ( m_bAlt & 0x80 ) );
}

inline int CHex::GetType() const {
    ASSERT_STRICT_VALID( this );

    return ( int( m_bType ) & 0x0F );
}

inline void CHex::SetBridge() {

    m_byType |= 0x80;
}

inline void CHex::ClrBridge() {

    m_byType &= 0x7f;
}

inline BOOL CHex::IsBridge() const {

    return 0 != ( m_byType & 0x80 );
}

inline BYTE CHex::GetVisibleType() {

    return ( m_byType & 0x78 ) >> 3;
}

inline void CHex::SetVisibleType( int iType ) {

    m_byType = ( m_byType & ~0x78 ) | ( ( iType << 3 ) & 0x78 );
}

inline int CHex::GetUnitDir() const {
    ASSERT_STRICT_VALID( this );
    return ( int( m_bType >> 4 ) & 0x0F );
}

inline void CHex::SetUnitDir( int iDir ) {
    ASSERT_STRICT( ( 0 <= iDir ) && ( iDir <= 0x0F ) );
    ASSERT_STRICT_VALID( this );

    m_bType = m_bType | ( ( iDir & 0x0F ) << 4 );
}

inline void CHex::ClrUnitDir() {
    m_bType = (BYTE)( m_bType & 0x0F );
}

inline void CHex::SetCursor() {
    m_bAlt |= 0x80;
}

inline void CHex::ClrCursor() {
    m_bAlt &= ~0x80;
}

inline void CHex::SetTree( int iTree ) {

    ASSERT( ( 0 <= iTree ) && ( iTree < 8 ) );
    m_bType = ( m_bType & 0x8F ) | (BYTE)( iTree << 4 );
}

inline int CHex::GetTree() const {

    return ( ( int( m_bType ) >> 4 ) & 0x07 );
}


/////////////////////////////////////////////////////////////////////////////
// CGameMap

inline CHex* CGameMap::GetHex( CHexCoord const& hex ) {
    return ( GetHex( hex.X(), hex.Y() ) );
}
inline CHex* CGameMap::GetHex( CSubHex const& _sub ) {
    return ( GetHex( _sub.x / 2, _sub.y / 2 ) );
}
inline CHex* CGameMap::GetHex( int x, int y ) {
    return ( _GetHex( CHexCoord::Wrap( x ), CHexCoord::Wrap( y ) ) );
}
inline CHex* CGameMap::_GetHex( CHexCoord const& hex ) {
    return ( _GetHex( hex.X(), hex.Y() ) );
}
inline CHex* CGameMap::_GetHex( CSubHex const& _sub ) {
    return ( _GetHex( _sub.x / 2, _sub.y / 2 ) );
}
inline CHex* CGameMap::_GetHex( int x, int y ) {
    ASSERT_STRICT( ( 0 <= x ) && ( x < m_eX ) );
    ASSERT_STRICT( ( 0 <= y ) && ( y < m_eY ) );
    CHex* pHex = m_pHex + ( y << m_iSideShift ) + x;
    ASSERT_STRICT_VALID( pHex );
    return pHex;
}

inline CHex const* CGameMap::GetHex( CHexCoord const& hex ) const {
    return ( GetHex( hex.X(), hex.Y() ) );
}
inline CHex const* CGameMap::GetHex( CSubHex const& _sub ) const {
    return ( GetHex( _sub.x / 2, _sub.y / 2 ) );
}
inline CHex const* CGameMap::GetHex( int x, int y ) const {
    return ( _GetHex( CHexCoord::Wrap( x ), CHexCoord::Wrap( y ) ) );
}
inline CHex const* CGameMap::_GetHex( CHexCoord const& hex ) const {
    return ( _GetHex( hex.X(), hex.Y() ) );
}
inline CHex const* CGameMap::_GetHex( CSubHex const& _sub ) const {
    return ( _GetHex( _sub.x / 2, _sub.y / 2 ) );
}
inline CHex const* CGameMap::_GetHex( int x, int y ) const {
    ASSERT_STRICT( ( 0 <= x ) && ( x < m_eX ) );
    ASSERT_STRICT( ( 0 <= y ) && ( y < m_eY ) );
    CHex* pHex = m_pHex + ( y << m_iSideShift ) + x;
    ASSERT_STRICT_VALID( pHex );
    return pHex;
}

inline CHex* CGameMap::_Xinc( CHex* pHex ) { return ( pHex + 1 ); }
inline CHex* CGameMap::_Xinc( CHex* pHex, int iNum ) { return ( pHex + iNum ); }
inline CHex* CGameMap::_Yinc( CHex* pHex ) { return ( pHex + m_eX ); }
inline CHex* CGameMap::_Yinc( CHex* pHex, int iNum ) { return ( pHex + iNum * m_eX ); }

inline int CGameMap::WrapX( int x ) const {
    while ( x >= m_eX )
        x -= m_eX;
    while ( x < 0 )
        x += m_eX;
    return ( x );
}
inline int CGameMap::WrapY( int y ) const {
    while ( y >= m_eY )
        y -= m_eY;
    while ( y < 0 )
        y += m_eY;
    return ( y );
}

// CTerrain
inline CTerrainData const& CTerrain::GetData( int iIndex ) const {
    ASSERT_STRICT( ( 0 <= iIndex ) && ( iIndex < CHex::num_types ) );
    ASSERT_STRICT_VALID( this );
    return ( m_Data[iIndex] );
}

#endif
