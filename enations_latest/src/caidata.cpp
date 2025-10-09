////////////////////////////////////////////////////////////////////////////
//
//  CAIData.cpp :  CAIData object implementation
//                 Divide and Conquer AI
//
//  Last update:    09/17/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "CAIData.hpp"

#include "CPathMap.h"
#include "stdafx.h"

extern CException* pException;  // standard exception for yielding

extern CStructure NEAR   theStructures;
extern CBuildingMap NEAR theBuildingMap;
extern CTransport NEAR   theTransports;
extern CVehicleMap NEAR  theVehicleMap;
extern CGameMap NEAR     theMap;
extern CTerrain NEAR     theTerrain;
extern CGame NEAR        theGame;

extern CRITICAL_SECTION cs;  // used by threads

extern CPathMap thePathMap;  // used by maps for pathing

#define new DEBUG_NEW


//
// return the time units for the vehicle type passed
// to move from a location to a destination location
//
int CAIData::GetRangeTime( CHexCoord& hFrom, CHexCoord& hTo, int iVehicle )
{
    ASSERT_VALID( this );

    // EnterCriticalSection (&cs);
    // int CGameMap::GetTravelTime( CHexCoord & hexSrc,
    //	CHexCoord & hexDest, int iVehType )
    //
    int iTime = theMap.GetTravelTime( hFrom, hTo, iVehicle );
    // LeaveCriticalSection (&cs);

    return ( iTime );
}
//
// return range in CHexes between locations
//
int CAIData::GetRangeDistance( CHexCoord& hFrom, CHexCoord& hTo )
{
    ASSERT_VALID( this );

    if ( hFrom == hTo )
        return ( 0 );

    int iDist = theMap.GetRangeDistance( hFrom, hTo );

    return ( iDist );
}

//
// get the terrain in at the passed location and
// return the slope value, no unit data is considered
// and this does not use critical sections for speed
//
int CAIData::GetCHexTerrain( CAIHex* pHex )
{
    CHexCoord getHex( pHex->m_iX, pHex->m_iY );
    int       iSlope   = 1;
    CHex*     pGameHex = theMap.GetHex( getHex );
    if ( pGameHex != NULL )
    {
        // extract the terrain type from low nibble
        const int iType  = pGameHex->GetType( );
        pHex->m_cTerrain = (BYTE)iType;
        // iSlope = pGameHex->GetSlope();
    }
    return ( iSlope );
}

//
// Get a pointer to a specific CHex, copy it into an CAIHex
// and return the CAIHex
//
int CAIData::GetCHexData( CAIHex* pHex )
{
    ASSERT_VALID( this );

    // mark the hex as unknown
    pHex->m_iUnit    = 0xFFFF;
    pHex->m_dwUnitID = (DWORD)0;
    pHex->m_cTerrain = (BYTE)0xFF;
    CHexCoord getHex( pHex->m_iX, pHex->m_iY );

    // default the returned value to sloped terrain (can't build)
    int iSlope = 0;

    EnterCriticalSection( &cs );

    CHex* pGameHex = theMap.GetHex( getHex );

    // known hex means return is not NULL
    if ( pGameHex != NULL )
    {
        // extract the terrain type from low nibble
        const int iType  = pGameHex->GetType( );
        pHex->m_cTerrain = (BYTE)iType;
        // iSlope = pGameHex->GetSlope();

        BYTE bUnits = pGameHex->GetUnits( );
        // there may be units in this CHex
        if ( bUnits != 0 )
        {
            // it is a hex of a building
            if ( bUnits & CHex::bldg )
            {
                CBuilding* pBldg = theBuildingHex.GetBuilding( getHex );
                if ( pBldg != NULL )
                {
                    // NOTE: this does not identify the type
                    // of building/vehicle unit, but indicates
                    // that the unit is a building or vehicle
                    pHex->m_iUnit    = pBldg->GetUnitType( );
                    pHex->m_dwUnitID = pBldg->GetID( );
                }
            }
            else  // its probably a vehicle
            {
                CSubHex   subHex( getHex.X( ), getHex.Y( ) );
                CVehicle* pVehicle = NULL;
                pHex->m_iUnit      = CUnit::vehicle;
                // BUGBUG right now there could be up to 4
                // vehicles that are partially in this hex
                // or one vehicle could be completely in this
                // hex and 2 others partially in the hex
                if ( bUnits & CHex::ul )
                {
                    pVehicle = theVehicleHex.GetVehicle( subHex );
                    if ( pVehicle != NULL )
                        pHex->m_dwUnitID = pVehicle->GetID( );
                }
                if ( bUnits & CHex::ur )
                {
                    subHex.x++;
                    pVehicle = theVehicleHex.GetVehicle( subHex );
                    if ( pVehicle != NULL )
                        pHex->m_dwUnitID = pVehicle->GetID( );
                }
                subHex.y++;
                if ( bUnits & CHex::lr )
                {
                    pVehicle = theVehicleHex.GetVehicle( subHex );
                    if ( pVehicle != NULL )
                        pHex->m_dwUnitID = pVehicle->GetID( );
                }
                if ( bUnits & CHex::ll )
                {
                    subHex.x--;
                    pVehicle = theVehicleHex.GetVehicle( subHex );
                    if ( pVehicle != NULL )
                        pHex->m_dwUnitID = pVehicle->GetID( );
                }
            }
        }
    }
    LeaveCriticalSection( &cs );
    return ( iSlope );
}

//
// BUGBUG CGame NEAR theGame does not appear to have an ineration by number
//
// CPlayer const * CAIData::GetNextPlayer (int iPlyrNum) const
CPlayer const* CAIData::GetNextPlayer( int ) const
{
    ASSERT_VALID( this );
    // BUGBUG default return
    return ( NULL );
}
//
// Get a pointer to a specific player
//
CPlayer* CAIData::GetPlayerData( int iPlyrNum ) const
{
    ASSERT_VALID( this );
    return ( (CPlayer*)theGame.GetPlayerByPlyr( iPlyrNum ) );
}
//
// inerate thru unit or building data
// passing dwID == NULL, return first instance known;
//         dwID != NULL, return instance known following dwID;
// always return id of next known; NULL to end
//
CBuilding const* CAIData::GetNextBuilding( int iPlyrNum, DWORD dwID ) const
{
    ASSERT_VALID( this );
    ASSERT_VALID( &theBuildingMap );
    DWORD      dwDumb;
    CBuilding* pBldg;

    POSITION pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
#if THREADS_ENABLED
        // BUGBUG this function must yield
        myYieldThread( );
        // if( myYieldThread() == TM_QUIT )
        //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        theBuildingMap.GetNextAssoc( pos, dwDumb, pBldg );
        ASSERT_VALID( pBldg );

        // consider only those building of this player
        if ( pBldg->GetOwner( )->GetPlyrNum( ) == iPlyrNum )
        {
            // get first
            if ( !dwID )
                return ( pBldg );

            // get next
            else if ( pBldg->GetID( ) > dwID )
                return ( pBldg );
        }
    }
    return ( NULL );
}
CVehicle const* CAIData::GetNextVehicle( int iPlyrNum, DWORD dwID ) const
{
    ASSERT_VALID( this );
    ASSERT_VALID( &theVehicleMap );
    CVehicle* pVehicle = NULL;
    DWORD     dwDumb;

    POSITION pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
#if THREADS_ENABLED
        // BUGBUG this function must yield
        myYieldThread( );
        // if( myYieldThread() == TM_QUIT )
        //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        theVehicleMap.GetNextAssoc( pos, dwDumb, pVehicle );
        // CVehicle *pVehicle = theVehicleList.GetNext (pos);
        ASSERT_VALID( pVehicle );

        // consider only those buildings of this player
        if ( pVehicle->GetOwner( )->GetPlyrNum( ) == iPlyrNum )
        {
            // get first
            if ( !dwID )
                return ( pVehicle );

            // get next
            else if ( pVehicle->GetID( ) > dwID )
                return ( pVehicle );
        }
    }
    return ( NULL );
}

void CAIData::GetBldgExit( DWORD dwID, CHexCoord& hex )
{
    EnterCriticalSection( &cs );
    CBuilding* pBldg = theBuildingMap.GetBldg( dwID );
    if ( pBldg != NULL )
        hex = pBldg->GetExitHex( );
    LeaveCriticalSection( &cs );
}

//
// get pointer to a specific unit or building
//
CBuilding* CAIData::GetBuildingData( int iPlyrNum, DWORD dwID )
{
    ASSERT_VALID( this );
    ASSERT_VALID( &theBuildingMap );

    CBuilding* pBldg = theBuildingMap.GetBldg( dwID );
    if ( pBldg != NULL )
    {
        if ( CAI_OPFOR_UNIT == iPlyrNum || pBldg->GetOwner( )->GetPlyrNum( ) == iPlyrNum )
            return ( pBldg );
    }
    return ( NULL );
}

//
// this function does not actually get a CBuilding
// but instead updates an existing CAIUnit with the
// id, type and owner of the building if it exists
//
int CAIData::GetBuilding( DWORD dwID, CAIUnit* paiUnit )
{
    int iType = CStructureData::num_types;

    EnterCriticalSection( &cs );

    CBuilding* pBldg = GetBuildingData( CAI_OPFOR_UNIT, dwID );
    if ( pBldg != NULL )
    {
        paiUnit->SetID( pBldg->GetID( ) );
        paiUnit->SetOwner( pBldg->GetOwner( )->GetPlyrNum( ) );
        paiUnit->SetType( pBldg->GetUnitType( ) );
        iType = pBldg->GetData( )->GetBldgType( );
    }

    LeaveCriticalSection( &cs );
    return ( iType );
}

void CAIData::GetVehicleHex( DWORD dwID, CHexCoord& hexUnit )
{
    ASSERT_VALID( this );
    ASSERT_VALID( &theVehicleMap );

    EnterCriticalSection( &cs );
    CVehicle* pVehicle = theVehicleMap.GetVehicle( dwID );
    if ( pVehicle != NULL )
        hexUnit = pVehicle->GetHexHead( );
    else
    {
        hexUnit.X( 0 );
        hexUnit.Y( 0 );
    }
    LeaveCriticalSection( &cs );
}

CVehicle* CAIData::GetVehicleData( int iPlyrNum, DWORD dwID )
{
    ASSERT_VALID( this );
    ASSERT_VALID( &theVehicleMap );

    CVehicle* pVehicle = theVehicleMap.GetVehicle( dwID );
    if ( pVehicle != NULL )
    {
        if ( CAI_OPFOR_UNIT == iPlyrNum || pVehicle->GetOwner( )->GetPlyrNum( ) == iPlyrNum )
            return ( pVehicle );
    }
    return ( NULL );
}

//
// this function does not actually get a CVehicle
// but instead updates an existing CAIUnit with the
// id, type and owner of the vehicle if it exists
//
int CAIData::GetVehicle( DWORD dwID, CAIUnit* paiUnit )
{
    int iType = CTransportData::num_types;

    EnterCriticalSection( &cs );

    CVehicle* pVehicle = GetVehicleData( CAI_OPFOR_UNIT, dwID );
    if ( pVehicle != NULL )
    {
        paiUnit->SetID( pVehicle->GetID( ) );
        paiUnit->SetOwner( pVehicle->GetOwner( )->GetPlyrNum( ) );
        paiUnit->SetType( pVehicle->GetUnitType( ) );
        iType = pVehicle->GetData( )->GetType( );
    }
    LeaveCriticalSection( &cs );
    return ( iType );
}

//
// get the player number of the unit just passed
//
int CAIData::GetOwnerID( DWORD dwID )
{
    EnterCriticalSection( &cs );

    CVehicle* pVehicle = GetVehicleData( CAI_OPFOR_UNIT, dwID );
    if ( pVehicle != NULL )
    {
        LeaveCriticalSection( &cs );
        return ( pVehicle->GetOwner( )->GetPlyrNum( ) );
    }
    CBuilding* pBldg = GetBuildingData( CAI_OPFOR_UNIT, dwID );
    if ( pBldg != NULL )
    {
        LeaveCriticalSection( &cs );
        return ( pBldg->GetOwner( )->GetPlyrNum( ) );
    }
    LeaveCriticalSection( &cs );

    return ( 0 );
}

//
// CHEAT
//
// this will find the building of the player passed
// and return the hex where it is located if the iBldg
// is non-zero; otherwise if it is zero, then there is
// a location in hexAt and we want the location that
// is closest to hexAt and return it there
// iBldg means building type
void CAIData::FindBuilding( int iBldg, int iPlayer, CHexCoord& hexAt )
{
    DWORD      dwDumb;
    CBuilding* pBldg;
    CHexCoord  hexBest = hexAt;
    int        iBest   = 0xFFFE;
    int        iDist;

    EnterCriticalSection( &cs );

    POSITION pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        theBuildingMap.GetNextAssoc( pos, dwDumb, pBldg );
        ASSERT_VALID( pBldg );

        // consider only those building of this player
        if ( pBldg->GetOwner( )->GetPlyrNum( ) == iPlayer )
        {
            if ( iBldg ) // if we're looking for a specific building type (0 means anything)
            {
                if ( pBldg->GetData( )->GetType( ) == iBldg )
                {
                    if ( iBldg == CStructureData::seaport || iBldg == CStructureData::shipyard_1 ||
                         iBldg == CStructureData::shipyard_3 )
                        hexAt = pBldg->GetShipHex( );
                    else
                        hexAt = pBldg->GetExitHex( );

                    LeaveCriticalSection( &cs );

                    #ifdef AI_SPECTATE
                    if (!pBldg->GetOwner()->IsAI())
                    {
                        TRAP( );
                    }
                    #endif

                    return;
                }
            }
            else
            {
                // only want buildings that are important
                if ( pBldg->GetData( )->GetType( ) != CStructureData::rocket &&
                     pBldg->GetData( )->GetType( ) < CStructureData::barracks_2 )
                    continue;

                CHexCoord hexBldg;
                if ( pBldg->GetData( )->GetType( ) == CStructureData::seaport ||
                     pBldg->GetData( )->GetType( ) == CStructureData::shipyard_1 ||
                     pBldg->GetData( )->GetType( ) == CStructureData::shipyard_3 )
                    hexBldg = pBldg->GetShipHex( );
                else
                    hexBldg = pBldg->GetExitHex( );
                iDist = GetRangeDistance( hexAt, hexBldg );
                if ( iDist && iDist < iBest )
                {
                    iBest   = iDist;
                    hexBest = hexBldg;
                }
            }
        }
    }
    LeaveCriticalSection( &cs );
#ifdef AI_SPECTATE
    if ( !pBldg->GetOwner( )->IsAI( ) )
    {
        TRAP( );
    }
#endif

    hexAt = hexBest;
}

//
// return the carrying capacity found for the unit indicated
//
int CAIData::GetMaterialCapacity( CAIUnit* paiUnit )
{
    int iQty = 0;

    EnterCriticalSection( &cs );

    // this code is being retained to allow future versions
    // to have buildings with a max capacity.  current tLP
    // allows unlimited storage at a building
#if 0
	if( paiUnit->GetType() == CUnit::building )
	{
		CBuilding *pBldg = theBuildingMap.GetBldg( paiUnit->GetID() );
		if( pBldg != NULL )
		{
			int iMax = pBldg->GetData()->GetMaxMaterials();
			int iStore = pBldg->GetTotalStore();
			iQty =  iMax - iStore;
		}
	}
#endif

    if ( paiUnit->GetType( ) == CUnit::vehicle )
    {
        CVehicle* pVehicle = theVehicleMap.GetVehicle( paiUnit->GetID( ) );
        if ( pVehicle != NULL )
            iQty = pVehicle->GetData( )->GetMaxMaterials( ) - pVehicle->GetTotalStore( );
    }

    LeaveCriticalSection( &cs );

    return ( iQty );
}

//
// return the quantity of the iMaterial found at the unit indicated
//
int CAIData::GetQtyMaterialAt( CAIUnit* paiUnit, int iMaterial )
{
    int iQty = 0;

    EnterCriticalSection( &cs );

    if ( paiUnit->GetType( ) == CUnit::building )
    {
        CBuilding* pBldg = GetBuildingData( paiUnit->GetOwner( ), paiUnit->GetID( ) );
        if ( pBldg != NULL )
            iQty = pBldg->GetStore( iMaterial );
    }
    else if ( paiUnit->GetType( ) == CUnit::vehicle )
    {
        CVehicle* pVehicle = GetVehicleData( paiUnit->GetOwner( ), paiUnit->GetID( ) );
        if ( pVehicle != NULL )
            iQty = pVehicle->GetStore( iMaterial );
    }

    LeaveCriticalSection( &cs );

    return ( iQty );
}

/*
// tell a vehicle to build a bridge - it must ALREADY be on it
// the ends must be on land
class _CMsgBridge : public CNetCmd
{
public:
        _CMsgBridge () : CNetCmd (-1) {}
        _CMsgBridge (int iMsg) : CNetCmd (iMsg) {}

        DWORD				m_dwIDBrdg;		// ID of bridge
        int					m_iPlyrNum;		// player requesting the placement
        DWORD				m_dwIDVeh;		// ID of vehicle
        CHexCoord		m_hexStart;		// location to build at
        CHexCoord		m_hexEnd;
        int					m_iAlt;				// bridge altitude
        int					m_iMode;
                        enum { one_hex, street };

#ifdef _DEBUG
public:
    void AssertValid() const;
#endif
};

// tell a vehicle to build a bridge - it must ALREADY be on it
class CMsgBuildBridge : public _CMsgBridge
{
public:
        CMsgBuildBridge () : _CMsgBridge (build_bridge) {}
        CMsgBuildBridge (CVehicle const *pVeh, CHexCoord const & hexEnd);

        void	ToErr () { m_iMsg = err_build_bridge; }
        CMsgBridgeNew *	ToNew () { m_iMsg = bridge_new; return ((CMsgBridgeNew *) this); }

#ifdef _DEBUG
public:
    void AssertValid() const;
#endif
};
*/

void CAIData::BuildBridgeAt( CAIUnit* paiUnit, CHexCoord& hexStart, CHexCoord& hexEnd )
{
    CMsgBuildBridge msg;
    msg.m_iPlyrNum = paiUnit->GetOwner( );
    msg.m_dwIDVeh  = paiUnit->GetID( );
    msg.m_hexStart = hexStart;
    msg.m_hexEnd   = hexEnd;
    theGame.PostToServer( &msg, sizeof( msg ) );
}

/*
// tell a vehicle to build a road - it must ALREADY be on it
class _CMsgRoad : public CNetCmd
{
public:
        _CMsgRoad () : CNetCmd (-1) {}
        _CMsgRoad (int iMsg) : CNetCmd (iMsg) {}

        int					m_iPlyrNum;		// player requesting the placement
        DWORD				m_dwID;				// ID of vehicle
        CHexCoord		m_hexBuild;		// location to build at
        int					m_iMode;
                        enum { one_hex, street };

#ifdef _DEBUG
public:
    void AssertValid() const;
#endif
};

// tell a vehicle to build a road - it must ALREADY be on it
class CMsgBuildRoad : public _CMsgRoad
{
public:
        CMsgBuildRoad () : _CMsgRoad (build_road) {}
        CMsgBuildRoad (CVehicle const *pVeh);

        void	ToErr () { m_iMsg = err_build_road; }
        CMsgRoadNew *	ToNew () { m_iMsg = road_new; return ((CMsgRoadNew *) this); }

#ifdef _DEBUG
public:
    void AssertValid() const;
#endif
};
*/
void CAIData::BuildRoadAt( CHexCoord& hexSite, CAIUnit* paiUnit )
{
    CMsgBuildRoad msg;
    msg.m_iPlyrNum = paiUnit->GetOwner( );
    msg.m_dwID     = paiUnit->GetID( );
    msg.m_hexBuild = hexSite;
    msg.m_iMode    = _CMsgRoad::one_hex;
    theGame.PostToServer( &msg, sizeof( msg ) );
}

/*
// tell a vehicle to build a building - it must ALREADY be adjacent
class CMsgBuildBldg : public CNetCmd
{
public:
        CMsgBuildBldg (CVehicle const *pVeh, CHexCoord const & hex, int iBldg);

        int					m_iPlyrNum;		// player requesting the placement
        DWORD				m_dwID;				// ID of vehicle
        int					m_X;					// location to build at
        int					m_Y;
        int					m_iType;			// building type to build

#ifdef _DEBUG
public:
    void AssertValid() const;
#endif
};
*/
void CAIData::BuildAt( CHexCoord& hexSite, int iBldg, int iDir, CAIUnit* paiUnit )
{
    EnterCriticalSection( &cs );

    CVehicle* pVehicle = theVehicleMap.GetVehicle( paiUnit->GetID( ) );
    if ( pVehicle != NULL )
    {
        // BUGBUG check the vehicle's location and make
        // sure it matches with the hexSite sent in msg
        CHexCoord hexVeh = pVehicle->GetHexHead( );
        ASSERT( hexVeh == hexSite );
        if ( hexVeh != hexSite )
        {
            LeaveCriticalSection( &cs );
            return;
        }

        CMsgBuildBldg msg( pVehicle, hexSite, iDir, iBldg );
        msg.m_iPlyrNum = paiUnit->GetOwner( );
        LeaveCriticalSection( &cs );
        theGame.PostToServer( &msg, sizeof( msg ) );
        return;
    }

    LeaveCriticalSection( &cs );
}
//
// temporary function that will clear the vehicle's event
// flag, to be used until Dave fixes the bug of construction
// being completed causing an ASSERT on re-assignment
//
void CAIData::ClearUnitEvent( CAIUnit* paiUnit )
{
    EnterCriticalSection( &cs );

    CVehicle* pVehicle = theVehicleMap.GetVehicle( paiUnit->GetID( ) );
    if ( pVehicle != NULL )
    {
        pVehicle->SetEvent( CVehicle::none );
    }

    LeaveCriticalSection( &cs );
}

BOOL CAIData::IsArtillery( int iVehType )
{
    BOOL bRet = FALSE;
    switch ( iVehType )
    {
    case CTransportData::light_art:
    case CTransportData::med_art:
    case CTransportData::heavy_art:
        bRet = TRUE;
    default:
        break;
    }
    return ( bRet );
}

//
// determines if the vehicle type passed
// is a land combat vehicle
//
BOOL CAIData::IsCombatVehicle( int iVehType )
{
    BOOL bRet = FALSE;
    switch ( iVehType )
    {
    case CTransportData::infantry_carrier:
    case CTransportData::light_tank:
    case CTransportData::med_tank:
    case CTransportData::heavy_tank:
    case CTransportData::heavy_scout:
    case CTransportData::light_art:
    case CTransportData::med_art:
    case CTransportData::heavy_art:
    case CTransportData::infantry:
    case CTransportData::rangers:
    case CTransportData::marines:
        bRet = TRUE;
    default:
        break;
    }
    return ( bRet );
}

//
// determine if the dwID passed belongs to a truck
//
BOOL CAIData::IsTruck( DWORD dwID )
{
    EnterCriticalSection( &cs );

    int       iVeh     = 0;
    CVehicle* pVehicle = theVehicleMap.GetVehicle( dwID );
    if ( pVehicle != NULL )
        iVeh = pVehicle->GetData( )->GetType( );

    LeaveCriticalSection( &cs );

    if ( iVeh == CTransportData::med_truck || iVeh == CTransportData::heavy_truck )
        return TRUE;

    return FALSE;
}

BOOL CAIData::IsLoaded( DWORD dwID )
{
    BOOL bIsLoaded = FALSE;
    EnterCriticalSection( &cs );
    CVehicle* pVehicle = theVehicleMap.GetVehicle( dwID );
    if ( pVehicle != NULL )
    {
        if ( pVehicle->GetTransport( ) != NULL )
            bIsLoaded = TRUE;
    }
    LeaveCriticalSection( &cs );
    return ( bIsLoaded );
}

//
// determine if the hex passed is the base hex
// of a CUnit that occupies multiple hexes
//
BOOL CAIData::IsHexOfUnit( CAIHex* pHex )
{
    CHexCoord hexTest( pHex->m_iX, pHex->m_iY );

    EnterCriticalSection( &cs );

    CVehicle* pVehicle = GetVehicleData( CAI_OPFOR_UNIT, pHex->m_dwUnitID );
    if ( pVehicle != NULL )
    {
        if ( pVehicle->GetHexHead( ) == hexTest )
        {
            LeaveCriticalSection( &cs );
            return TRUE;
        }
    }
    CBuilding* pBldg = GetBuildingData( CAI_OPFOR_UNIT, pHex->m_dwUnitID );
    if ( pBldg != NULL )
    {
        if ( pBldg->GetHex( ) == hexTest )
        {
            LeaveCriticalSection( &cs );
            return TRUE;
        }
    }
    LeaveCriticalSection( &cs );

    return FALSE;
}
//
// inerate thru terrain type data
// passing iIndex == NULL, return first instance;
//         iIndex != NULL, return instance following iIndex;
//
CTerrainData const& CAIData::GetNextTerrain( int iIndex ) const
{
    ASSERT_VALID( this );
    if ( !iIndex )
        return ( theTerrain.GetData( iIndex ) );
    if ( ( iIndex + 1 ) < CHex::num_types )
        return ( theTerrain.GetData( ( iIndex + 1 ) ) );
    return ( theTerrain.GetData( 0 ) );
}

CTerrainData const* CAIData::GetTerrainData( int iIndex ) const
{
    ASSERT_VALID( this );
    if ( iIndex < CHex::num_types )
        return ( &theTerrain.GetData( iIndex ) );
    // BUGBUG always return something valid?
    return ( &theTerrain.GetData( 0 ) );
}
//
// inerate thru building type data
// passing iIndex == NULL, return first instance;
//         iIndex != NULL, return instance following iIndex;
//
CStructureData const* CAIData::GetNextStructure( int iIndex ) const
{
    ASSERT_VALID( this );
    if ( !iIndex )
        return ( theStructures.GetData( iIndex ) );
    if ( ( iIndex + 1 ) < theStructures.GetNumBuildings( ) )
        return ( theStructures.GetData( ( iIndex + 1 ) ) );
    return NULL;
}
//
// get pointer to a specific building type
//
CStructureData const* CAIData::GetStructureData( int iIndex ) const
{
    ASSERT_VALID( this );
    if ( iIndex < theStructures.GetNumBuildings( ) )
        return ( theStructures.GetData( iIndex ) );
    return NULL;
}
//
// inerate thru vehicle type data
// passing iIndex == NULL, return first instance;
//         iIndex != NULL, return instance following iIndex
//
CTransportData const* CAIData::GetNextTransport( int iIndex ) const
{
    ASSERT_VALID( this );
    if ( !iIndex )
        return ( theTransports.GetData( iIndex ) );
    if ( ( iIndex + 1 ) < theTransports.GetNumTransports( ) )
        return ( theTransports.GetData( ( iIndex + 1 ) ) );
    return NULL;
}
//
// get pointer to a specific vehicle type
//
CTransportData const* CAIData::GetTransportData( int iIndex ) const
{
    ASSERT_VALID( this );
    if ( iIndex < theTransports.GetNumTransports( ) )
        return ( theTransports.GetData( iIndex ) );
    return NULL;
}

//
// this needs to be called at the start of any game
// and after loading a new game
//
void CAIData::SetWorldSize( int iHexPerBlk, int iBlkPerSide )
{
    ASSERT_VALID( this );
    m_iHexPerBlk  = iHexPerBlk;
    m_iBlkPerSide = iBlkPerSide;
    // make the map path mgr support the entire map
    int iSize = iHexPerBlk * iBlkPerSide;

    // BUGBUG this is no longer correct
    // initialize map's pathing utility
    if ( theGame.AmServer( ) )
        thePathMap.Init( iSize, iSize );
}

//
// create
//
CAIData::CAIData( int iSmart, int iNumAi, int iNumHuman )
{
    m_iSmart         = iSmart;
    m_iNumAi         = iNumAi;
    m_iNumHuman      = iNumHuman;
    m_iNumBuildings  = theStructures.GetNumBuildings( );
    m_iNumTransports = theTransports.GetNumTransports( );

    // determine the largest sized structure that will
    // be dealt with in widthX and widthY hexes
    m_iStructureSize = 0;
    for ( int i = 0; i < m_iNumBuildings; ++i )
    {
        CStructureData const* pData = theStructures.GetData( i );
        if ( pData->GetCX( ) > m_iStructureSize )
            m_iStructureSize = pData->GetCX( );
        if ( pData->GetCY( ) > m_iStructureSize )
            m_iStructureSize = pData->GetCY( );
    }
    // BUGBUG force max size due to bad data in CStructureData
    // m_iStructureSize = 2;

    // set up an array of AI hexes to store the best density
    // values by mineral type, and its location
    m_paihDensity  = NULL;
    m_paihDensity  = new CAIHex[CMaterialTypes::num_types];
    CAIHex* paiHex = NULL;
    for ( int i = 0; i < CMaterialTypes::num_types; ++i )
    {
        paiHex = &m_paihDensity[i];

        paiHex->m_iX       = 0;  // location where it is
        paiHex->m_iY       = 0;
        paiHex->m_iUnit    = 0;
        paiHex->m_dwUnitID = (DWORD)0;  // quantity there
        paiHex->m_cTerrain = (BYTE)0;   // record best density here
    }


    // separate seeding screws up multi-player
#if 0
	// seed the random number generator
	DWORD dwTicks = timeGetTime();
	while( dwTicks >= (DWORD)RAND_MAX )
		dwTicks /= (DWORD)RAND_MAX;
	srand( (unsigned)dwTicks );
#endif

    ASSERT_VALID( this );
}

CAIData::~CAIData( )
{
    if ( m_paihDensity != NULL )
        delete[] m_paihDensity;
#if 0
	CString sDesc;

#if 0
	for( int i=0; i<m_iNumTransports; ++i )
	{
		CTransportData const *pData
			= theTransports.GetData( i );
		sDesc = pData->GetDesc();
		logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"Vehicle %d - %s range %d attack %d,%d,%d ",
			i, (const char *)sDesc, pData->_GetRange(),
			pData->_GetAttack(0),pData->_GetAttack(1),pData->_GetAttack(2) );
		logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"type=%d  def=%d  rate=%d  acc=%d ",
			pData->GetTargetType(),pData->_GetDefense(),
			pData->_GetFireRate(),pData->_GetAccuracy() );
		logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"spotting=%d  hits=%d  wheel=%d  water=%d \n",
			pData->_GetSpottingRange(),pData->GetDamagePoints(),
			pData->GetWheelType(),pData->GetWaterDepth() );

		sDesc.Empty();
	}
	for( i=0; i<m_iNumBuildings; ++i )
	{
		CStructureData const *pData
			= theStructures.GetData( i );
		sDesc = pData->GetDesc();
		logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"Building %d - %s range %d attack %d,%d,%d ",
			i, (const char *)sDesc, pData->_GetRange(),
			pData->_GetAttack(0),pData->_GetAttack(1),pData->_GetAttack(2) );
		logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"type=%d  def=%d  rate=%d  acc=%d ",
			pData->GetTargetType(),pData->_GetDefense(),
			pData->_GetFireRate(),pData->_GetAccuracy() );
		logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
			"spotting=%d  hits=%d  \n",
			pData->_GetSpottingRange(),pData->GetDamagePoints() );

		sDesc.Empty();
	}
#endif
	//TRACE( " \n" );

	for( int j=0; j<NUM_WHEEL_TYPES; ++j )
	{
		for( int i=0; i<CHex::num_types; ++i )
		{
			CTerrainData const *pTerrain = &theTerrain.GetData(i);
			if( pTerrain == NULL )
				break;
		
			logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH,  
				"Terrain %d costs %d for wheel type %d ",
				i, pTerrain->GetWheelMult(j), j );
		}

		//TRACE( " \n" );
	}
#endif
}

//
// returns a random number less than or equal to iDice
//
int CAIData::GetRandom( int iDice )
{
    // use Dave's random number routine
    return ( RandNum( iDice ) );
}

// Extended this to remove the logging stuff below
#if 0
//
// returns a random number less than or equal to iDice
//
int CAIData::GetRandom( int iDice )
{
	int dran,sd,dr;
	if( iDice <= 0 )
		return( iDice );
	sd = (int)(RAND_MAX/iDice);
	dran = rand();
	if( dran & 0x1000 )
		dran = rand();
	dr = dran/sd;
	if( dr > iDice )
		dr = iDice;
	return( dr );
}
#endif  // if 0

//#if _LOGOUT
/*
enum {	        cmd_ready,                      // we're ready to start
                cmd_you_are,                    // passes player number, get ready to start game
                cmd_player,                     // each opponent
                cmd_start,						// start the game (build world data)
                cmd_plyr_status,				// tell other players this players status
                cmd_init_done,					// this player is ready to go
                cmd_chat,						// chat before the game starts
                cmd_play,						// all players read - start playing
                cmd_to_ai,						// human player becomes AI controlled
                cmd_to_hp,						// AI player becomes human controlled
                cmd_pause,						// pause the game
                cmd_resume,						// resume the game

                cmd_enum_plyrs,					// to server asking for players in game
                cmd_plyr_join,					// players avail to join (load game)
                cmd_select_plyr,				// select a player to play
                cmd_select_ok,					// selection was accepted
                cmd_select_not_ok,
                cmd_game_data,					// data for loading a game
                cmd_plyr_taken,					// loaded player taken

                // sent to server
                place_bldg,						// initial placement of a building
                place_veh,						// initial placement of a vehicle
                build_bldg,						// build a building
                build_veh,						// build a vehicle
                build_road,						// build a road on a hex
                build_bridge,					// build a bridge
                veh_goto,						// tell a vehicle to go somewhere
                trans_mat,						// transfer materials between units
                unit_control,					// stop/resume/destroy a unit
                unit_damage,					// notify of damage level
                unit_repair,					// repair unit
                destroy_unit,					// destroy a unit
                stop_destroy_unit,			    // stop destroying a unit
                load_carrier,					// load units on a carrier
                unload_carrier,					// unload all units from a carrier

                // sent from server
                bldg_new,						// a new building is created
                bldg_stat,						// built/damage status of a bldg
                veh_new,						// a new vehicle is created
                veh_stat,						// damage status of a vehicle
                veh_loc,						// vehicle going to the next hex
                veh_dest,						// vehicle has arrived at its destination
                road_new,						// start a new road hex
                road_done,						// a new road hex is completed
                bridge_new,						// start a new bridge
                bridge_done,					// a new bridge is completed
                unit_set_damage,				// received damage level
                unit_set_repair,				// received damage level (repaired)
                unit_destroying,				// a unit is being destroyed
                stop_unit_destroying,		    // a unit is stopped being destroyed
                delete_unit,					// a unit is gone (sent by owner)
                deploy_it,						// deploy vehicle
                plyr_dying,						// player is dying - kill all units
                scenario,						// scenario message to AI - scenario starting
                scenario_atk,					// scenario message to - atack specific HP units
                unit_loaded,					// unit loaded itself
                unit_repaired,					// unit repaired
                unit_attacked,					// unit was attacked
                build_civ,						// sent to AI to build a civ building

                // sent by players to themselves
                plyr_dead,						// player is dead - delete him

                // messages from AI to server
                veh_set_dest,					// set dest
                attack,							// attack a unit
                set_rsrch,						// set R&D to research
                see_unit,						// see a unit
                repair_veh,						// repair a vehicle
                repair_bldg,					// repair/help build a bldg

                                                // errors sent from server
                err_place_bldg,
                err_build_bldg,
                err_build_veh,
                err_build_road,
                err_build_bridge,
                err_veh_goto,
                err_veh_traffic,

                ipc_msg,						// generic chat message
                ai_msg,							// msg from non-server to AI player

                // adding new ones down here so eric doesn't have to change constants
                out_of_LOS,						// your target can't be hit anymore
                set_relations,					// set relations
                shoot_gun,						// shooting at something
                game_speed,						// client set's speed
                bldg_materials,					// materials a building has
                cmd_get_file,					// get the game file (load net game)
                cmd_new_server,					// new system becoming server
80				give_unit,						// give unit to new owner
                set_time,						// tell clients game time

                last_message					// used for ASSERT
                };

*/
const char* CAIData::GetMsgString( int iMsg )
{
    switch ( iMsg )
    {
    case CNetCmd::cmd_ready:
        return ( "cmd_ready" );
    case CNetCmd::cmd_you_are:
        return ( "cmd_you_are" );
    case CNetCmd::cmd_player:
        return ( "cmd_player" );
    case CNetCmd::cmd_start:
        return ( "cmd_start" );
    case CNetCmd::cmd_play:
        return ( "cmd_play" );
    case CNetCmd::cmd_pause:
        return ( "cmd_pause" );
    case CNetCmd::cmd_resume:
        return ( "cmd_resume" );
    case CNetCmd::bldg_new:
        return ( "bldg_new" );
    case CNetCmd::bldg_stat:
        return ( "bldg_stat" );
    case CNetCmd::veh_new:
        return ( "veh_new" );
    case CNetCmd::veh_stat:
        return ( "veh_stat" );
    case CNetCmd::veh_loc:
        return ( "veh_loc" );
    case CNetCmd::veh_goto:
        return ( "veh_goto" );
    case CNetCmd::road_new:
        return ( "road_new" );
    case CNetCmd::road_done:
        return ( "road_done" );
    case CNetCmd::unit_damage:
        return ( "unit_damage" );
    case CNetCmd::unit_set_damage:
        return ( "unit_set_damage" );
    case CNetCmd::unit_set_repair:
        return ( "unit_set_repair" );
    case CNetCmd::load_carrier:
        return ( "load_carrier" );
    case CNetCmd::unload_carrier:
        return ( "unload_carrier" );
    case CNetCmd::veh_set_dest:
        return ( "veh_set_dest" );
    case CNetCmd::veh_dest:
        return ( "veh_dest" );
    case CNetCmd::plyr_dying:
        return ( "plyr_dying" );
    case CNetCmd::plyr_dead:
        return ( "plyr_dead" );
    case CNetCmd::scenario:
        return ( "scenario" );
    case CNetCmd::scenario_atk:
        return ( "scenario_atk" );
    case CNetCmd::unit_destroying:
        return ( "unit_destroying" );
    case CNetCmd::delete_unit:
        return ( "delete_unit" );
    case CNetCmd::out_of_LOS:
        return ( "out_of_LOS" );
        // case CNetCmd::set_release_owner:
        return ( "set_release_owner" );
    case CNetCmd::deploy_it:
        return ( "deploy_it" );
    // case CNetCmd::set_blocked:
    //	return( "set_blocked" );
    case CNetCmd::see_unit:
        return ( "see_unit" );
    case CNetCmd::unit_attacked:
        return ( "unit_attacked" );
    case CNetCmd::unit_repaired:
        return ( "unit_repaired" );
    case CNetCmd::unit_loaded:
        return ( "unit_loaded" );
    case CNetCmd::repair_veh:
        return ( "repair_veh" );
    case CNetCmd::repair_bldg:
        return ( "repair_bldg" );
    case CNetCmd::build_civ:
        return ( "build_civ" );
    case CNetCmd::err_place_bldg:
        return ( "err_place_bldg" );
    case CNetCmd::err_build_bldg:
        return ( "err_build_bldg" );
    case CNetCmd::err_build_veh:
        return ( "err_build_veh" );
    case CNetCmd::err_build_road:
        return ( "err_build_road" );
    case CNetCmd::err_veh_goto:
        return ( "err_veh_goto" );
    case CNetCmd::err_veh_traffic:
        return ( "err_veh_traffic" );
    case CNetCmd::bridge_new:
        return ( "bridge_new" );
    case CNetCmd::bridge_done:
        return ( "bridge_done" );
    case CNetCmd::set_relations:
        return ( "set_relations" );
    case CNetCmd::shoot_gun:
        return ( "shoot_gun" );
    case CNetCmd::bldg_materials:
        return ( "bldg_materials" );
    case CNetCmd::attack:
        return ( "attack" );
    case CNetCmd::give_unit:
        return ( "give_unit" );
    default:
        return ( "unknown" );
    }
}
//#endif  // _LOGOUT


// CAIData.cpp
