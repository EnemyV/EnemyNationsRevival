#ifndef __ENNAVVIEW_H__
#define __ENNAVVIEW_H__

////////////////////////////////////////////////////////////////////////////
//
//  ennavview.h : the navigation read seam for the vehicle-movement A*.
//
//  Every live-world read CPathMgr::_GetPath and the helpers it calls during a
//  search make goes through CEnNavView. One implementation exists today - the
//  live one below, which reads theMap / theTerrain / theTransports /
//  theVehicleHex / theBuildingHex / theBridgeHex exactly as the search read
//  them before - so that a snapshot-backed view can later be handed to a worker
//  thread without a second copy of the search.
//
//  Non-virtual and header-only on purpose: the accessors sit in the innermost
//  A* loop, and nothing here may cost an indirect call.
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "terrain.inl"
#include "building.inl"
#include "vehicle.inl"
#include "bridge.h"
#include "pathworld.h"

// What the search knows about one hex. Values, never a CHex*: a worker reading a
// snapshot has no live hex to point at.
struct CEnHexFacts
{
    BOOL bValid;
    BYTE bUnits;
    int  iType;
    int  iAlt;

    CEnHexFacts( ): bValid( FALSE ), bUnits( 0 ), iType( 0 ), iAlt( 0 ) {}

    BOOL IsValid( ) const { return ( bValid ); }
    BYTE GetUnits( ) const { return ( bUnits ); }
    int  GetType( ) const { return ( iType ); }
    int  GetAlt( ) const { return ( iAlt ); }
    BOOL IsWater( ) const
    {
        return ( ( iType == CHex::river ) || ( iType == CHex::ocean ) || ( iType == CHex::lake ) );
    }
};

// Enough of a bridge for the orphan-mark, same-bridge and deck-direction rules.
// uParent is an opaque identity token, equal for two hexes of one span and 0 for
// none - never dereferenced.
struct CEnBridgeFacts
{
    BOOL   bExists;
    BOOL   bParentBuilt;
    int    iExit;
    size_t uParent;

    CEnBridgeFacts( ): bExists( FALSE ), bParentBuilt( FALSE ), iExit( -1 ), uParent( 0 ) {}
};

// Enough of a building for the entrance/exit rules. uKey is an opaque identity
// token, 0 for none.
struct CEnBuildingFacts
{
    BOOL      bExists;
    size_t    uKey;
    CHexCoord hexExit;
    int       iExitDir;
    CHexCoord hexShip;
    int       iShipDir;

    CEnBuildingFacts( )
        : bExists( FALSE ), uKey( 0 ), hexExit( 0, 0 ), iExitDir( -1 ), hexShip( 0, 0 ), iShipDir( -1 )
    {
    }
};

class CEnNavView
{
  public:
    // Marks a navigation view for the template overloads in vehicle.h: without it
    // CanEnterHex( view, src, dest, ... ) also deduces for CSubHex arguments and
    // becomes ambiguous against the four-argument live signature.
    typedef void EnNavViewTag;

    CEnHexFacts GetHex( CHexCoord const& hex ) const
    {
        CEnHexFacts f;
        CHex const* pHex = theMap.GetHex( hex );
        if ( pHex == NULL )
            return ( f );
        ASSERT_STRICT_VALID( pHex );
        f.bValid = TRUE;
        f.bUnits = pHex->GetUnits( );
        f.iType  = pHex->GetType( );
        f.iAlt   = pHex->GetAlt( );
        return ( f );
    }

    CEnBridgeFacts GetBridge( CHexCoord const& hex ) const
    {
        CEnBridgeFacts f;
        CBridgeUnit* pBu = theBridgeHex.GetBridge( hex );
        if ( pBu == NULL )
            return ( f );
        f.bExists     = TRUE;
        f.iExit       = pBu->GetExit( );
        CBridge* pPar = pBu->GetParent( );
        f.uParent     = (size_t)pPar;
        f.bParentBuilt = ( ( pPar != NULL ) && pPar->IsBuilt( ) );
        return ( f );
    }

    CEnBuildingFacts GetBuilding( CHexCoord const& hex ) const
    {
        CEnBuildingFacts f;
        CBuilding const* pBldg = theBuildingHex._GetBuilding( hex );
        if ( pBldg == NULL )
            return ( f );
        f.bExists  = TRUE;
        f.uKey     = (size_t)pBldg;
        f.hexExit  = pBldg->GetExitHex( );
        f.iExitDir = pBldg->GetExitDir( );
        f.hexShip  = pBldg->GetShipHex( );
        f.iShipDir = pBldg->GetShipDir( );
        return ( f );
    }

    int GetWheelMult( int iTerrainType, int iWheel ) const
    {
        return ( theTerrain.GetData( iTerrainType ).GetWheelMult( iWheel ) );
    }

    int GetTerrainCost( CHexCoord const& hexFrom, CHexCoord const& hexTo, int iDir, int iWheel ) const
    {
        return ( theMap.GetTerrainCost( hexFrom, hexTo, iDir, iWheel ) );
    }

    int GetRangeDistance( CHexCoord const& hex1, CHexCoord const& hex2 ) const
    {
        CHexCoord h1( hex1 );
        CHexCoord h2( hex2 );
        return ( theMap.GetRangeDistance( h1, h2 ) );
    }

    CTransportData const* GetTransportData( int iVehType ) const { return ( theTransports.GetData( iVehType ) ); }

    // Is every vehicle occupying this hex on the move? Such a hex is passable for
    // planning purposes; ordinary movement must still wait for actual clearance.
    // Traffic-wait mode is transient. Explicit Stop, parked and blocked modes are
    // obstacles.
    BOOL IsHexMovingVehicle( CHexCoord const& hex ) const
    {
        // ALL FOUR sub-hexes: a hex is passable only if everything sitting in it is
        // actually driving. Checking just the first occupant accepted a hex whose
        // other half was parked, which is exactly the obstacle we must not plan through.
        BOOL bAny = FALSE;

        for ( int iX = 0; iX < 2; iX++ )
            for ( int iY = 0; iY < 2; iY++ )
            {
                CVehicle* pVeh = theVehicleHex._GetVehicle( CSubHex( hex.X( ) * 2 + iX, hex.Y( ) * 2 + iY ) );
                if ( pVeh == NULL )
                    continue;

                if ( pVeh->IsFlag( CUnit::stopped ) || !pVeh->IsOnTheMove( ) )
                    return ( FALSE );  // something is parked here - a real obstacle
                bAny = TRUE;
            }

        return ( bAny );
    }

    int TrafficRules( ) const { return ( TrafficOpts( ) ); }

    int Wrap( int iVal ) const { return ( CHexCoord::Wrap( iVal ) ); }
    int Diff( int iVal ) const { return ( CHexCoord::Diff( iVal ) ); }
};

// The live view under the name the search templates use for it.
typedef CEnNavView CEnLiveNavView;

// The same accessor set over an immutable PathWorld. Same shape, same names, same
// return types as CEnLiveNavView and no virtuals, so a search instantiated on this
// costs exactly what the live one costs.
//
// GetTerrainCost and GetRangeDistance below MIRROR CGameMap::_GetTerrainCost
// (terrain.cpp) and CGameMap::GetRangeDistance (CHexCoord overload) line for line,
// including every special case. They are copies, not improvements: a difference of
// one case is a route difference the shadow gate exists to catch.
class CEnSnapNavView
{
  public:
    typedef void EnNavViewTag;

    explicit CEnSnapNavView( PathWorld const& pw ): m_pw( pw ) {}

    PathWorld const& World( ) const { return ( m_pw ); }

    CEnHexFacts GetHex( CHexCoord const& hex ) const
    {
        CEnHexFacts             f;
        PathWorld::Hex const&   h = m_pw.At( hex.X( ), hex.Y( ) );
        // CGameMap::GetHex masks both axes and indexes: it never returns NULL, so the
        // live view's bValid is TRUE for every coordinate the search can form.
        f.bValid = TRUE;
        f.bUnits = h.bUnits;
        f.iType  = h.bType;
        f.iAlt   = h.bAlt;
        return ( f );
    }

    CEnBridgeFacts GetBridge( CHexCoord const& hex ) const
    {
        CEnBridgeFacts            f;
        PathWorld::Bridge const*  p = m_pw.FindBridge( hex.X( ), hex.Y( ) );
        if ( p == NULL )
            return ( f );
        f.bExists      = TRUE;
        f.iExit        = p->iExit;
        f.uParent      = p->uParent;
        f.bParentBuilt = p->bParentBuilt;
        return ( f );
    }

    CEnBuildingFacts GetBuilding( CHexCoord const& hex ) const
    {
        CEnBuildingFacts        f;
        PathWorld::Bldg const*  p = m_pw.FindBldg( hex.X( ), hex.Y( ) );
        if ( p == NULL )
            return ( f );
        f.bExists  = TRUE;
        f.uKey     = p->uKey;
        f.hexExit  = p->hexExit;
        f.iExitDir = p->iExitDir;
        f.hexShip  = p->hexShip;
        f.iShipDir = p->iShipDir;
        return ( f );
    }

    int GetWheelMult( int iTerrainType, int iWheel ) const { return ( m_pw.WheelMult( iTerrainType, iWheel ) ); }

    int GetTerrainCost( CHexCoord const& hexFrom, CHexCoord const& hexTo, int iDir, int iWheel ) const
    {
        PathWorld::Hex const& hxFrom = m_pw.At( hexFrom.X( ), hexFrom.Y( ) );
        PathWorld::Hex const* pDest  = &m_pw.At( hexTo.X( ), hexTo.Y( ) );
        CHexCoord             hexDest( hexTo );
        if ( hexFrom == hexTo )
        {
            pDest = &hxFrom;
            iDir  = 0;
        }

        int iTyp;
        if ( pDest->bUnits & CHex::bldg )
            iTyp = CHex::city;
        else if ( pDest->bUnits & CHex::bridge )
            iTyp = ( iWheel == CWheelTypes::water ) ? (int)pDest->bType : (int)CHex::road;
        else
            iTyp = pDest->bType;

        int iRtn = m_pw.WheelMult( iTyp, iWheel );
        if ( iRtn == 0 )
        {
            if ( ( iWheel == CWheelTypes::water ) && ( pDest->bUnits & CHex::bldg ) )
            {
                PathWorld::Bldg const* pBldg = m_pw.FindBldg( hexDest.X( ), hexDest.Y( ) );
                if ( ( pBldg != NULL ) && pBldg->bShipExit )
                    return ( 1 );
            }
            if ( ( iWheel == CWheelTypes::water ) && ( iTyp == CHex::river ) )
                iRtn = m_pw.WheelMult( CHex::ocean, iWheel );
            else if ( ( iWheel == CWheelTypes::water ) && ( iTyp == CHex::coastline ) )
                iRtn = m_pw.WheelMult( CHex::ocean, iWheel ) * 8;
            if ( iRtn == 0 )
                return ( 0 );
        }

        if ( iDir & 1 )
            iRtn *= 3;
        else
            iRtn *= 2;

        int iAlt = (int)pDest->bAlt - (int)hxFrom.bAlt;
        if ( iAlt >= 0 )
            iRtn += ( iRtn * iAlt ) / 8;
        else
        {
            iRtn -= ( iRtn * iAlt ) / 16;
            iRtn = __max( 1, iRtn );
        }

        return ( iRtn );
    }

    int GetRangeDistance( CHexCoord const& hex1, CHexCoord const& hex2 ) const
    {
        int x = abs( m_pw.Diff( hex1.X( ) - hex2.X( ) ) );
        int y = abs( m_pw.Diff( hex1.Y( ) - hex2.Y( ) ) );

        int iStraight = abs( x - y );
        int iDiag     = __min( x, y );
        return ( ( 3 * iDiag + 2 * iStraight + 1 ) / 2 );
    }

    // PINNED, not copied: theTransports is loaded with the gameplay data and is not
    // written for the lifetime of a world (a reload goes through CloseWorld, which
    // drops every snapshot). The search holds the returned pointer in m_pTD exactly
    // as it does for a live search, and a vehicle-driven search takes the same
    // pointer from pVehicle->GetData() either way.
    CTransportData const* GetTransportData( int iVehType ) const { return ( theTransports.GetData( iVehType ) ); }

    BOOL IsHexMovingVehicle( CHexCoord const& hex ) const
    {
        return ( m_pw.At( hex.X( ), hex.Y( ) ).bOcc == PathWorld::occ_moving ? TRUE : FALSE );
    }

    int TrafficRules( ) const { return ( m_pw.TrafficRules( ) ); }

    int Wrap( int iVal ) const { return ( m_pw.Wrap( iVal ) ); }
    int Diff( int iVal ) const { return ( m_pw.Diff( iVal ) ); }

  private:
    PathWorld const& m_pw;
};

#endif  // __ENNAVVIEW_H__
