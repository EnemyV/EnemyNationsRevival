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

#endif  // __ENNAVVIEW_H__
