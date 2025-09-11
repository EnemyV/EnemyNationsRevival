
//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// unit.cpp : buildings & vehicles
//

#include "stdafx.h"
#include "base.h"
#include "lastplnt.h"
#include "cpathmgr.h"
#include "netapi.h"
#include "player.h"
#include "sprite.h"
#include "chproute.hpp"
#include "icons.h"
#include "bitmaps.h"
#include "area.h"
#include "bridge.h"
#include "event.h"
#include "netcmd.h"

#include "minerals.inl"
#include "terrain.inl"
#include "unit.inl"
#include "building.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

//------------------------------ I s o m e t r i c I n i t ------------------

static BYTE GabyIsometricTable[128];

class IsometricInit
{
  public:
    IsometricInit( );
};

static IsometricInit g_isometricinit;

//---------------------------------------------------------------------------
// IsometricInit::IsometricInit
//---------------------------------------------------------------------------
IsometricInit::IsometricInit( )
{
    for ( int i = 0; i < 128; ++i )
    {
        // Convert dir to angle

        double dSin;
        double dAngle;
        BYTE   byAngle;

        if ( i <= 32 )
        {
            dSin    = i / 32.;
            dAngle  = asin( dSin );
            byAngle = 0.5 + ( 32. * dAngle / ( 3.14159265 / 2. ) );
        }
        else if ( i <= 64 )
        {
            byAngle = 32 + 32 - GabyIsometricTable[64 - i];
        }
        else if ( i <= 96 )
        {
            byAngle = 64 + GabyIsometricTable[i - 64];
        }
        else
        {
            byAngle = 96 + 32 - GabyIsometricTable[128 - i];
        }

        GabyIsometricTable[i] = byAngle & 0x007f;
    }
}

void UnshowBuilding( CBuilding* pBldg );


CMap<DWORD, DWORD, CUnit*, CUnit*> theUnitMap;

CStructure     theStructures( "building" );
CEffect        theEffects( "effect" );
CBuildingMap   theBuildingMap;
CTransport     theTransports( "vehicle" );
CVehicleMap    theVehicleMap;
CBuildingHex   theBuildingHex;
CVehicleHex    theVehicleHex;
CStructureType theStructureType;
CTurrets       theTurrets( "turret" );
CMuzzleFlashes theFlashes( "muzzle" );

//---------------------------------------------------------------------------
// ToIsometricDir
//---------------------------------------------------------------------------
static BYTE ToIsometricDir( BYTE byDir, CVehicleSprite::TILT_TYPE eTilt )
{
    switch ( eTilt )
    {
    case CVehicleSprite::TILT_BACK:
        return 48;
    case CVehicleSprite::TILT_FLAT:
        return GabyIsometricTable[( 48 - byDir ) & 0x007f];
    case CVehicleSprite::TILT_FORWARD:
        return ( 48 - byDir ) & 0x007f;

    default:
        ASSERT( 0 );
    }

    return 0;
}

//---------------------------------------------------------------------------
// AdjustDirection
//---------------------------------------------------------------------------
static void AdjustDirection( CQuadDrawParms& drawparms, int iTiltIndex, int iDirectionIndex, int iDir )
{
    // Adjust angle due to tweening between sprite views

    int iSpriteDir          = ( EIGHTH_ROT * ( iDirectionIndex + 2 * xiDir ) ) & 0x007f;
    int iSpriteDirIsometric = ToIsometricDir( iSpriteDir, CVehicleSprite::TILT_TYPE( iTiltIndex ) );
    int iDirIsometric       = ToIsometricDir( iDir, CVehicleSprite::TILT_TYPE( iTiltIndex ) );
    int iDeltaIsometric     = iSpriteDirIsometric - iDirIsometric;

    if ( iDeltaIsometric < -63 )
        iDeltaIsometric += 128;

    if ( iDeltaIsometric > 63 )
        iDeltaIsometric -= 128;

    BYTE byTurnAngle;

#define SMOOTH_VEHICLE_TURNING
#ifdef SMOOTH_VEHICLE_TURNING
    byTurnAngle = iDeltaIsometric * 2;  // convert to 256 degrees
#else
    byTurnAngle            = 0;
#endif

    // Adjust angle due to terrain

    int iViewDir = ( iDir - ( xiDir * QUARTER_ROT ) ) & 0x007f;
    int iWeight  = 32 - abs( 32 - abs( 64 - ( ( iViewDir - EIGHTH_ROT ) & 0x007f ) ) );

    if ( 128 > drawparms.m_byRotAngle )
        drawparms.m_byRotAngle = (BYTE)( (unsigned)drawparms.m_byRotAngle * iWeight / 32 );
    else
        drawparms.m_byRotAngle = (BYTE)( 256 - ( ( 256 - drawparms.m_byRotAngle ) * iWeight / 32 ) );

    drawparms.m_byRotAngle -= byTurnAngle;
}

/////////////////////////////////////////////////////////////////////////////
// CTransportData - info about vehicle types

CTransportData::TRANS_BASE_TYPE CTransportData::GetBaseType( ) const
{

    switch ( m_iType )
    {
    case construction:
    case med_truck:
    case heavy_truck:
        return ( non_combat );

    case light_scout:
    case med_scout:
    case heavy_scout:
    case infantry_carrier:
    case light_tank:
    case med_tank:
    case heavy_tank:
        return ( combat );

    case light_art:
    case med_art:
    case heavy_art:
        return ( artillery );

    case light_cargo:
    case gun_boat:
    case destroyer:
    case cruiser:
    case landing_craft:
        return ( ship );

    case infantry:
    case rangers:
    case marines:
        return ( troops );
    }

    return ( non_combat );
}

// return TRUE if can enter pHexDest from pHexSrc
//  this code assumes the hexes are adjoining (diaganol ok)
//  bStrict == FALSE - can enter sides of buildings/bridge ends
// WARNING - does not check for entering enemy building (doesn't know the vehicle)
BOOL CTransportData::CanEnterHex( CHexCoord const& hexSrc, CHexCoord const& hexDest, BOOL bVehOnWater,
                                  BOOL bStrict ) const
{

    ASSERT( ( abs( CHexCoord::Diff( hexSrc.X( ) - hexDest.X( ) ) ) <= 1 ) &&
            ( abs( CHexCoord::Diff( hexSrc.Y( ) - hexDest.Y( ) ) ) <= 1 ) );
    CHex* pHexSrc  = theMap.GetHex( hexSrc );
    CHex* pHexDest = theMap.GetHex( hexDest );

    // see if we can move to the dest
    if ( !CanTravelHex( pHexDest ) )
        return ( FALSE );

    // if the same hex we are done
    if ( hexSrc == hexDest )
        return ( TRUE );

    // if not a bridge or building - leave
    if ( ( 0 == ( pHexSrc->GetUnits( ) & ( CHex::bldg | CHex::bridge ) ) ) &&
         ( 0 == ( pHexDest->GetUnits( ) & ( CHex::bldg | CHex::bridge ) ) ) )
        return ( TRUE );

    // very special case - if dest is a bridge and is not water then it's a single hex island
    // which is a support or end from the water and nothing can enter it.
    if ( bVehOnWater && ( !pHexDest->IsWater( ) ) && ( pHexDest->GetUnits( ) & CHex::bridge ) )
        return ( FALSE );

    // movement
    int xDif = CHexCoord::Diff( hexDest.X( ) - hexSrc.X( ) );
    int yDif = CHexCoord::Diff( hexDest.Y( ) - hexSrc.Y( ) );

    // only if we are not on water do we care about a bridge
    if ( !bVehOnWater )
    {
        // ships can't enter a bridge (and better not be on one)
        if ( ( pHexDest->GetUnits( ) & CHex::bridge ) && ( GetWheelType( ) == CWheelTypes::water ) )
        {
            TRAP( );
            return ( FALSE );
        }

        CBridgeUnit* pBuDest = theBridgeHex.GetBridge( hexDest );

        // if on a bridge either same bridge or exit
        if ( pHexSrc->GetUnits( ) & CHex::bridge )
        {
            CBridgeUnit* pBuSrc = theBridgeHex.GetBridge( hexSrc );
            // both bridge - same bridge check (or exit one enter other below)
            if ( pHexDest->GetUnits( ) & CHex::bridge )
                if ( pBuSrc->GetParent( ) == pBuDest->GetParent( ) )
                    return ( TRUE );

            // is our exit direction ok? (if it is we continue in case going to a bridge or bldg)
            // only care if strict
            if ( !bStrict )
            {
                TRAP( );
                if ( pBuSrc->GetExit( ) == -1 )
                {
                    TRAP( );
                    return ( FALSE );
                }
                TRAP( );
            }
            else

                switch ( pBuSrc->GetExit( ) )
                {
                case 0:  // exit up
                    if ( yDif >= 0 )
                        return ( FALSE );
                    break;
                case 1:  // exit right
                    if ( xDif <= 0 )
                        return ( FALSE );
                    break;
                case 2:  // exit down
                    if ( yDif <= 0 )
                        return ( FALSE );
                    break;
                case 3:  // exit left
                    if ( xDif >= 0 )
                        return ( FALSE );
                    break;
                default:  // if we are not on an exit
                    return ( FALSE );
                }
        }

        // if entering a bridge - are we coming from the right direction
        if ( pHexDest->GetUnits( ) & CHex::bridge )
        {
            // note - we are the reverse of the dir because we are entering
            // only care if strict
            if ( !bStrict )
            {
                TRAP( );
                if ( pBuDest->GetExit( ) == -1 )
                {
                    TRAP( );
                    return ( FALSE );
                }
                TRAP( );
            }
            else

                switch ( pBuDest->GetExit( ) )
                {
                case 0:  // enter from the down
                    if ( yDif <= 0 )
                        return ( FALSE );
                    break;
                case 1:  // enter from the left
                    if ( xDif >= 0 )
                        return ( FALSE );
                    break;
                case 2:  // enter from the up
                    if ( yDif >= 0 )
                        return ( FALSE );
                    break;
                case 3:  // enter from the right
                    if ( xDif <= 0 )
                        return ( FALSE );
                    break;
                default:  // if we are not on an entrance
                    return ( FALSE );
                }
        }
    }  // bVehOnWater

    // if we had bridges above we can leave now if no buildings
    if ( ( 0 == ( pHexSrc->GetUnits( ) & CHex::bldg ) ) && ( 0 == ( pHexDest->GetUnits( ) & CHex::bldg ) ) )
        return ( TRUE );

    // bridge stuff done - now the building stuff
    CBuilding* pBldgSrc  = theBuildingHex._GetBuilding( hexSrc );
    CBuilding* pBldgDest = theBuildingHex._GetBuilding( hexDest );

    // building to building - must be the same
    if ( ( pHexSrc->GetUnits( ) & CHex::bldg ) && ( pHexDest->GetUnits( ) & CHex::bldg ) && ( pBldgSrc != pBldgDest ) )
        return ( FALSE );

    // if not strict we're done
    if ( !bStrict )
        return ( TRUE );

    // dest building must be going to the exit
    if ( pBldgDest != NULL )
        if ( hexDest != ( IsBoat( ) ? pBldgDest->GetShipHex( ) : pBldgDest->GetExitHex( ) ) )
            return ( FALSE );

    // leaving a building
    if ( pHexSrc->GetUnits( ) & CHex::bldg )
    {
        switch ( IsBoat( ) ? pBldgSrc->GetShipDir( ) : pBldgSrc->GetExitDir( ) )
        {
        case 0:   // exit up
            return ( yDif < 0 );
        case 1:   // exit right
            return ( xDif > 0 );
        case 2:   // exit down
            return ( yDif > 0 );
        case 3:   // exit left
            return ( xDif < 0 );
        default:  // this is impossible
            TRAP( );
            return ( FALSE );
        }
    }

    // entering a building
    switch ( IsBoat( ) ? pBldgDest->GetShipDir( ) : pBldgDest->GetExitDir( ) )
    {
    case 0:  // enter from the top
        return ( yDif > 0 );
    case 1:  // enter from the right
        return ( xDif < 0 );
    case 2:  // enter from the down
        return ( yDif < 0 );
    case 3:  // enter from the left
        return ( xDif > 0 );
    }

    TRAP( );
    return ( FALSE );
}

// return TRUE if can travel on this hex
BOOL CTransportData::CanTravelHex( CHex const* pHex ) const
{

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( pHex );

    // if a building we can travel on it
    if ( ( pHex->GetUnits( ) & CHex::bldg ) != 0 )
        return ( TRUE );

    // handle land units first (most likely)
    if ( m_cWheelType != CWheelTypes::water )
    {
        // if a completed bridge we can travel on it
        if ( ( pHex->GetUnits( ) & CHex::bridge ) != 0 )
        {
            if ( theBridgeHex.GetBridge( pHex->GetHex( ) )->GetParent( )->IsBuilt( ) )
                return ( TRUE );
            return ( FALSE );
        }

        switch ( pHex->GetType( ) )
        {
        case CHex::lake:
        case CHex::ocean:
            return ( GetWaterDepth( ) > CHex::sea_level - pHex->GetAlt( ) );
        case CHex::river:
            return ( GetWaterDepth( ) > 0 );
        default:
            if ( theTerrain.GetData( pHex->GetType( ) ).GetWheelMult( m_cWheelType ) != 0 )
                return ( TRUE );
            return ( FALSE );
        }
    }

    //   not water - we can travel on coastline tiles
    if ( ( pHex->GetType( ) != CHex::lake ) && ( pHex->GetType( ) != CHex::ocean ) )
    {
        if ( ( IsCarrier( ) ) && ( GetWaterDepth( ) == 0 ) &&
             ( theTerrain.GetData( pHex->GetType( ) ).GetWheelMult( m_cWheelType ) != 0 ) )
            return ( TRUE );
        return ( FALSE );
    }

    return ( GetWaterDepth( ) <= CHex::sea_level - pHex->GetAlt( ) );
}

//-----------------------------C F l a m e S p o t --------------------------

//---------------------------------------------------------------------------
// CFlameSpot::SetAge
//---------------------------------------------------------------------------
void CFlameSpot::SetAge( int iAgePercent )
{
    // VTTODO: this assert was failing when loaidng games..
   // ASSERT( 0 <= iAgePercent && iAgePercent <= 100 );

    ASSERT( 0 <= SMOKE_START_PERCENT && SMOKE_START_PERCENT <= 100 );
    ASSERT( 0 <= FLAME_START_PERCENT && FLAME_START_PERCENT <= 100 );

    if ( iAgePercent < SMOKE_START_PERCENT )
        m_ptrtileSmoke = NULL;
    else
    {
        if ( !m_ptrtileSmoke.Value( ) )
            m_ptrtileSmoke = new CEffectTile;

        int nSmoke = theEffects.GetCount( CEffect::smoke );

        ASSERT( 0 < nSmoke );

        int iIndex = ( iAgePercent - SMOKE_START_PERCENT ) * nSmoke / ( 101 - SMOKE_START_PERCENT );

        m_ptrtileSmoke->m_psprite = theEffects.GetSprite( CEffect::smoke, iIndex );
    }

    if ( iAgePercent < FLAME_START_PERCENT )
        m_ptrtileFlame = NULL;
    else
    {
        if ( !m_ptrtileFlame.Value( ) )
            m_ptrtileFlame = new CEffectTile;

        int nFlame = theEffects.GetCount( CEffect::fire );

        ASSERT( 0 < nFlame );

        int iIndex = ( iAgePercent - FLAME_START_PERCENT ) * nFlame / ( 101 - FLAME_START_PERCENT );

        m_ptrtileFlame->m_psprite = theEffects.GetSprite( CEffect::fire, iIndex );
    }
}

//---------------------------------------------------------------------------
// CFlameSpot::Draw
//---------------------------------------------------------------------------
CRect CFlameSpot::Draw( CPoint ptHotSpotClient )
{
    CRect rectBound( 0, 0, 0, 0 );

    CSpriteView* pspriteviewFlame = NULL;
    CSpriteView* pspriteviewSmoke = NULL;

    if ( m_ptrtileFlame.Value( ) )
        pspriteviewFlame = m_ptrtileFlame->GetView( );

    if ( m_ptrtileSmoke.Value( ) )
    {
        pspriteviewSmoke = m_ptrtileSmoke->GetView( );

        CPoint ptAnchor = pspriteviewSmoke->GetAnchorPoint( );
        CPoint ptOffset = ptHotSpotClient - ptAnchor;

        // Shift up the smoke if there's flame here

        if ( pspriteviewFlame )
            ptOffset.y -= pspriteviewFlame->Height( ) >> 1;

        CDrawParms drawparms( *m_ptrtileSmoke, ptOffset );

        rectBound |= pspriteviewSmoke->Draw( drawparms );
    }

    if ( pspriteviewFlame )
    {
        CPoint ptAnchor = pspriteviewFlame->GetAnchorPoint( );
        CPoint ptOffset = ptHotSpotClient - ptAnchor;

        CDrawParms drawparms( *m_ptrtileFlame, ptOffset );

        rectBound |= pspriteviewFlame->Draw( drawparms );
    }

    return rectBound;
}

//---------------------------------------------------------------------------
// CFlameSpot::IsHit
//---------------------------------------------------------------------------
BOOL CFlameSpot::IsHit( CPoint ptHotSpotClient, CPoint ptCursor ) const
{
    CSpriteView* pspriteviewFlame = NULL;
    CSpriteView* pspriteviewSmoke = NULL;

    if ( m_ptrtileFlame.Value( ) )
        pspriteviewFlame = m_ptrtileFlame->GetView( );

    if ( m_ptrtileSmoke.Value( ) )
    {
        pspriteviewSmoke = m_ptrtileSmoke->GetView( );

        CPoint ptAnchor = pspriteviewSmoke->GetAnchorPoint( );
        CPoint ptOffset = ptHotSpotClient - ptAnchor;

        // Shift up the smoke if there's flame here

        if ( pspriteviewFlame )
            ptOffset.y -= pspriteviewFlame->Height( ) >> 1;

        CDrawParms drawparms( *m_ptrtileSmoke, ptOffset );

        if ( pspriteviewSmoke->IsHit( drawparms, ptCursor ) )
            return TRUE;
    }

    if ( pspriteviewFlame )
    {
        CPoint ptAnchor = pspriteviewFlame->GetAnchorPoint( );
        CPoint ptOffset = ptHotSpotClient - ptAnchor;

        CDrawParms drawparms( *m_ptrtileFlame, ptOffset );

        if ( pspriteviewFlame->IsHit( drawparms, ptCursor ) )
            return TRUE;
    }

    return FALSE;
}

//------------------------ C D a m a g e D i s p l a y ----------------------

//---------------------------------------------------------------------------
// CDamageDisplay::CDamageDisplay
//---------------------------------------------------------------------------
CDamageDisplay::CDamageDisplay( CSprite const& sprite )
{
    m_nSpots     = sprite.GetNumHotSpots( CHotSpotKey::SMOKE_FLAME_HOTSPOT );
    m_pflamespot = new CFlameSpot[m_nSpots];
}

//---------------------------------------------------------------------------
// CDamageDisplay::~CDamageDisplay
//---------------------------------------------------------------------------
CDamageDisplay::~CDamageDisplay( )
{
    delete[] m_pflamespot;
}

//---------------------------------------------------------------------------
// CDamageDisplay::SetDamage
//---------------------------------------------------------------------------
void CDamageDisplay::SetDamage( int iPercent )
{
    for ( int i = 0; i < m_nSpots; ++i )
    {
        int iAgeStart = 1 == m_nSpots ? 0 : i * LAST_START_PERCENT / ( m_nSpots - 1 );

        if ( iAgeStart > iPercent )
        {
            for ( int j = i; j < m_nSpots; ++j ) m_pflamespot[j].SetAge( -1 );

            return;
        }

        int iAgePercent = ( iPercent - iAgeStart ) * 100 / ( 100 - iAgeStart );

        m_pflamespot[i].SetAge( iAgePercent );
    }
}

//---------------------------------------------------------------------------
// CDamageDisplay::Draw
//---------------------------------------------------------------------------
CRect CDamageDisplay::Draw( CSpriteView const& spriteview, CDrawParms const& drawparms )
{
    CRect rectBound( 0, 0, 0, 0 );

    for ( int i = 0; i < m_nSpots; ++i )
    {
        CHotSpotKey     hotspotkey( CHotSpotKey::SMOKE_FLAME_HOTSPOT, i );
        CHotSpot const* photspot = spriteview.GetHotSpot( hotspotkey );

        if ( !photspot )
            continue;

        CPoint ptHotSpotClient = spriteview.GetHotSpotPoint( *photspot );

        ptHotSpotClient = drawparms.SpriteViewToWindow( ptHotSpotClient, spriteview );  // Convert to client coords

        rectBound |= m_pflamespot[i].Draw( ptHotSpotClient );
    }

    return rectBound;
}

//---------------------------------------------------------------------------
// CDamageDisplay::IsHit
//---------------------------------------------------------------------------
BOOL CDamageDisplay::IsHit( CSpriteView const& spriteview, CDrawParms const& drawparms, CPoint ptCursor ) const
{
    for ( int i = 0; i < m_nSpots; ++i )
    {
        CHotSpotKey     hotspotkey( CHotSpotKey::SMOKE_FLAME_HOTSPOT, i );
        CHotSpot const* photspot = spriteview.GetHotSpot( hotspotkey );

        if ( !photspot )
            continue;

        CPoint ptHotSpotClient = spriteview.GetHotSpotPoint( *photspot );

        ptHotSpotClient = drawparms.SpriteViewToWindow( ptHotSpotClient, spriteview );  // Convert to client coords

        if ( m_pflamespot[i].IsHit( ptHotSpotClient, ptCursor ) )
            return TRUE;
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CUnit - base for CBuilding & CVehicle

void CUnit::_SetOwner( CPlayer* pPlyr )
{

    // no change
    if ( pPlyr == GetOwner( ) )
        return;

    // if lose alliance, take away
    int iRel = GetOwner( )->GetTheirRelations( );
    if ( ( GetUnitType( ) == CUnit::building ) && ( iRel == RELATIONS_ALLIANCE ) &&
         ( pPlyr->GetTheirRelations( ) != RELATIONS_ALLIANCE ) )
    {
        CBuilding* pBldg = (CBuilding*)this;
        if ( ( pBldg->GetData( )->GetUnionType( ) == CStructureData::UTmine ) ||
             ( pBldg->GetData( )->GetType( ) == CStructureData::lumber ) )
            theGame.m_pHpRtr->MsgTakeBldg( pBldg );
    }

    // combat and spotting off
    RemoveUnit( );

    // assign new owner & flag
    m_pOwner = pPlyr;
    GetFlag( )->Init( pPlyr->GetPlyrNum( ) );

    // set up spotting, oppo fire, etc.
    AddUnit( );

    // if gain alliance we get to share
    if ( ( GetUnitType( ) == CUnit::building ) && ( iRel != RELATIONS_ALLIANCE ) &&
         ( pPlyr->GetTheirRelations( ) == RELATIONS_ALLIANCE ) )
    {
        CBuilding* pBldg = (CBuilding*)this;
        if ( ( pBldg->GetData( )->GetUnionType( ) == CStructureData::UTmine ) ||
             ( pBldg->GetData( )->GetType( ) == CStructureData::lumber ) )
            theGame.m_pHpRtr->MsgGiveBldg( pBldg );
    }
}

int CUnit::GetDamageInflicted( CUnit const* pTarget, BOOL bRand ) const
{

    // can we shoot
    if ( GetFireRate( ) <= 0 )
        return ( 0 );

    int iDamage = GetAttack( 0 ) - pTarget->GetDefense( );

    if ( bRand )
    {
        if ( iDamage <= 0 )
            iDamage = 0;
        else if ( iDamage == 1 )
            iDamage = RandNum( 1 );
        else
            iDamage = iDamage / 2 + RandNum( iDamage / 2 );

        if ( pTarget->GetData( )->GetTargetType( ) == CUnitData::naval )
            iDamage += ( GetAttack( 2 ) + RandNum( GetAttack( 2 ) ) ) / 2;
        else
            iDamage += ( GetAttack( 1 ) + RandNum( GetAttack( 1 ) ) ) / 2;
    }

    else
    {
        iDamage = __max( iDamage, 0 );
        if ( pTarget->GetData( )->GetTargetType( ) == CUnitData::naval )
            iDamage += GetAttack( 2 );
        else
            iDamage += GetAttack( 1 );
    }

    if ( iDamage < 0 )
        return ( 0 );
    return ( iDamage );
}

// handle sharing resources
void CBuilding::UpdateStore( BOOL bForce )
{

    CBuilding* pBldg = (CBuilding*)this;

    // did we change a remote building (give material)?
    if ( !GetOwner( )->IsLocal( ) )
    {
        m_iUpdateMat = 0;
        CMsgBldgMat msg( pBldg, FALSE );
        theGame.PostToClient( GetOwner( ), &msg, sizeof( msg ) );
        return;
    }

    // only update our buildings to allies
    if ( ( GetUnitType( ) != CUnit::building ) || ( !theGame.IsNetGame( ) ) || ( !theGame.AnyAlliances( ) ) )
    {
        m_iUpdateMat = 0;
        return;
    }

    // if we aren't forced to update (increase in stores) and it's been under 30 sec
    // 10 seconds for buildings under construction
    if ( bForce || ( ( m_iUpdateMat > 1 ) && ( !IsConstructing( ) ) ) ||
         ( m_dwLastMatTime + 30 * 1000 < theGame.GettimeGetTime( ) ) ||
         ( ( m_iUpdateMat > 1 ) || ( m_dwLastMatTime + 10 * 1000 < theGame.GettimeGetTime( ) ) ) )
    {
        m_iUpdateMat    = 0;
        m_dwLastMatTime = theGame.GettimeGetTime( );
        CMsgBldgMat msg( pBldg, TRUE );

        // tell our friends
        POSITION pos;
        for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
        {
            CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
            if ( ( !pPlr->IsLocal( ) ) && ( pPlr->GetRelations( ) == RELATIONS_ALLIANCE ) )
                theGame.PostToClient( pPlr, &msg, sizeof( msg ) );
        }
    }
}

int CUnit::GetStoreDiff( int iInd, BOOL bReset )
{

    int iRtn = m_aiStore[iInd] - m_aiBeforeUpdate[iInd];

    if ( bReset )
        m_aiBeforeUpdate[iInd] = m_aiStore[iInd];

    return iRtn;
}

void CUnit::StoreMsg( CMsgBldgMat* pMsg )
{

    // if remote, we're getting the new totals
    if ( !GetOwner( )->IsLocal( ) )
    {
        for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ )
        {
            if ( pMsg->m_bTotal )
            {
                m_aiStore[iInd]        = pMsg->m_aiMat[iInd];
                m_aiBeforeUpdate[iInd] = 0;
            }
            else
            // save the difference
            {
                int iDif               = m_aiStore[iInd] - m_aiBeforeUpdate[iInd];
                m_aiStore[iInd]        = pMsg->m_aiMat[iInd];
                m_aiBeforeUpdate[iInd] = pMsg->m_aiMat[iInd] + iDif;
            }
        }

        MaterialChange( );
        return;
    }

    // we're getting a change
    TRAP( pMsg->m_bTotal );
    for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ )
    {
        if ( pMsg->m_bTotal )
            m_aiStore[iInd] = pMsg->m_aiMat[iInd];
        else
            m_aiStore[iInd] += pMsg->m_aiMat[iInd];
        // check for too much taken
        TRAP( m_aiStore[iInd] < 0 );
        m_aiStore[iInd] = __max( 0, m_aiStore[iInd] );
    }
    MaterialChange( );
}

// zero out a remote building's store
void CUnit::ZeroStore( )
{

    memset( &m_aiStore, 0, sizeof( m_aiStore ) );
    memset( &m_aiBeforeUpdate, 0, sizeof( m_aiBeforeUpdate ) );
    m_iUpdateMat = 0;
}

void CUnit::IncSee( CUnit* pSpotter )
{

    if ( m_pdwPlyrsSee[pSpotter->GetOwner( )->GetPlyrNum( )]++ != 0 )
        return;

    // tell the AI
    if ( pSpotter->GetOwner( )->IsAI( ) )
    {
#ifdef _LOGOUT
        logPrintf( LOG_PRI_USEFUL, LOG_ATTACK, "AI Unit %d sees unit %d", pSpotter->GetID( ), GetID( ) );
#endif
        ASSERT( theGame.AmServer( ) );
        CMsgSeeUnit msg( pSpotter, this );
        theGame.PostToClient( pSpotter->GetOwner( ), &msg, sizeof( msg ) );
        return;
    }

#ifdef _LOGOUT
    logPrintf( LOG_PRI_USEFUL, LOG_ATTACK, "HP Unit %d sees unit %d", pSpotter->GetID( ), GetID( ) );
#endif

    // if it's a building we tell the user
    if ( pSpotter->GetOwner( )->IsMe( ) )
        if ( pSpotter->GetUnitType( ) == CUnit::building )
            if ( GetOwner( )->GetRelations( ) >= RELATIONS_NEUTRAL )
                theGame.Event( EVENT_BLDG_ENEMY_CLOSE, EVENT_NOTIFY, pSpotter );
}

void CUnit::SetSelected( BOOL bDoList )
{

    SetFlag( selected );

    if ( bDoList )
    {
        if ( GetUnitType( ) == building )
        {
            int iInd = theApp.m_wndBldgs.FindItem( this );
            if ( iInd >= 0 )
                theApp.m_wndBldgs.m_ListBox.SetSel( iInd, TRUE );
        }
        else
        {
            int iInd = theApp.m_wndVehicles.FindItem( this );
            if ( iInd >= 0 )
                theApp.m_wndVehicles.m_ListBox.SetSel( iInd, TRUE );
        }
    }
}

void CUnit::SetUnselected( BOOL bDoList )
{

    ClrFlag( selected );

    if ( bDoList )
    {
        if ( GetUnitType( ) == building )
        {
            int iInd = theApp.m_wndBldgs.FindItem( this );
            if ( iInd >= 0 )
                theApp.m_wndBldgs.m_ListBox.SetSel( iInd, FALSE );
        }
        else
        {
            int iInd = theApp.m_wndVehicles.FindItem( this );
            if ( iInd >= 0 )
                theApp.m_wndVehicles.m_ListBox.SetSel( iInd, FALSE );
        }
    }
}

// we decrement the visibility counter for all hexes this unit can see
void CUnit::DecrementSpotting( )
{

    // if not spotting - we're done
    if ( !m_bSpotted )
        return;

    DWORD*    pdwSpot = m_dwaSpot;
    CHexCoord _hex( m_hexSpotting );
    int       x        = _hex.X( );
    int       iNumBlks = SPOTTING_ARRAY_SIZE;

    m_bSpotted = FALSE;

    // make sure we are decrementing where we incremented!!!
    ASSERT( ( m_dwaSpot[MAX_SPOTTING] == 0 ) || ( m_hexSpotting == _hex ) );

    while ( iNumBlks-- > 0 )
    {
        if ( *pdwSpot != 0 )
        {
            int   iInd = 1;
            CHex* pHex = theMap._GetHex( _hex );

            for ( int iNum = 32; iNum > 0; iNum-- )
            {
                if ( *pdwSpot & iInd )
                {
                    ASSERT( pHex->GetVisibility( ) );
                    pHex->DecVisible( );
                    if ( !pHex->GetVisibility( ) )
                    {
                        _hex.SetInvalidated( );

                        // turn off ambients for buildings
                        if ( pHex->GetUnits( ) & CHex::bldg )
                        {
                            CBuilding* pBldg = theBuildingHex._GetBuilding( _hex );
                            if ( ( pBldg != NULL ) && ( pBldg->GetOwner( ) != GetOwner( ) ) )
                                pBldg->PauseAnimations( TRUE );
                        }
                    }
                }

                iInd <<= 1;
                _hex.Xinc( );
                if ( _hex.X( ) == 0 )
                    pHex = theMap._GetHex( _hex );
                else
                    pHex = theMap._Xinc( pHex );
            }
        }

        _hex.X( ) = x;
        _hex.Yinc( );
        pdwSpot++;
    }
}

// we increment the visibility counter for all hexes this unit can see
void CUnit::IncrementSpotting( CHexCoord const& hex )
{

    // only me
    if ( ( m_pOwner == NULL ) || ( !m_pOwner->IsMe( ) ) )
    {
        TRAP( );
        return;
    }

    // if spotted - we're done
    if ( m_bSpotted )
        return;

    DWORD*    pdwSpot = m_dwaSpot;
    CHexCoord _hex( hex.X( ) - MAX_SPOTTING, hex.Y( ) - MAX_SPOTTING );
    _hex.Wrap( );
    int x        = _hex.X( );
    int iNumBlks = SPOTTING_ARRAY_SIZE;

    m_bSpotted    = TRUE;
    m_hexSpotting = _hex;

    while ( iNumBlks-- > 0 )
    {
        if ( *pdwSpot != 0 )
        {
            int   iInd = 1;
            CHex* pHex = theMap._GetHex( _hex );

            for ( int iNum = 32; iNum > 0; iNum-- )
            {

                if ( *pdwSpot & iInd )
                {
                    pHex->IncVisible( );

                    // if just became visible
                    if ( pHex->GetVisibility( ) == 1 )
                    {
                        _hex.SetInvalidated( );
                        if ( ( pHex->GetType( ) == CHex::road ) && ( pHex->GetVisibleType( ) != CHex::road ) )
                            pHex->ChangeToRoad( _hex );

                        if ( pHex->GetUnits( ) & CHex::bldg )
                        {
                            CBuilding* pBldg = theBuildingHex._GetBuilding( _hex );
                            if ( ( pBldg != NULL ) && ( pBldg->GetOwner( ) != GetOwner( ) ) )
                            {
                                // did it die?
                                if ( pBldg->IsFlag( CUnit::dead ) )
                                {
                                    if ( theApp.m_pLogFile != NULL )
                                    {
                                        TRAP( );
                                        char sBuf[80];
                                        sprintf( sBuf, "Dead building visible %d at %d,%d", pBldg->GetID( ),
                                                 pBldg->GetHex( ).X( ), pBldg->GetHex( ).Y( ) );
                                        theApp.Log( sBuf );
                                    }
                                    delete pBldg;
                                }
                                else

                                {
                                    if ( !pBldg->IsVisible( ) )
                                        pBldg->MakeBldgVisible( );

                                    // building just became visible so we update all of this
                                    pBldg->SetConstPer( );
                                    pBldg->UpdateDamageLevel( );
                                    if ( ( !pBldg->IsConstructing( ) ) && ( !pBldg->IsFlag( stopped ) ) )
                                    {
                                        if ( !pBldg->GetAmbient( CSpriteView::ANIM_FRONT_1 )->IsEnabled( ) )
                                            pBldg->EnableAnimations( TRUE );
                                        pBldg->PauseAnimations( FALSE );
                                    }
                                }
                            }
                        }
                    }


                    // check to see if this is a scenario objective
                    if ( ( theGame.GetScenario( ) == 4 ) && ( GetOwner( )->IsMe( ) ) &&
                         ( pHex->GetUnits( ) & CHex::bldg ) )
                    {
                        CBuilding* pBldg = theBuildingHex._GetBuilding( _hex );
                        if ( ( pBldg != NULL ) && ( pBldg->GetOwner( )->IsAI( ) ) )
                            for ( int iInd = 0; iInd < 5; iInd++ )
                                if ( pBldg->GetID( ) == theGame.m_adwScenarioUnits[iInd] )
                                {
                                    theGame.m_adwScenarioUnits[iInd] = 0;
                                    UnshowBuilding( pBldg );
                                    break;
                                }
                    }
                }

                iInd <<= 1;
                _hex.Xinc( );
                if ( _hex.X( ) == 0 )
                    pHex = theMap._GetHex( _hex );
                else
                    pHex = theMap._Xinc( pHex );
            }
        }

        _hex.X( ) = x;
        _hex.Yinc( );
        pdwSpot++;
    }
}

void CUnit::OppoAndOthers( )
{

    // if doesn't own hexes it can't be involved
    if ( GetUnitType( ) == CUnit::vehicle )
        if ( !( (CVehicle*)this )->GetHexOwnership( ) )
        {
            TRAP( );
            return;
        }

    DetermineOppo( );
    OtherOppo( );
}

// increment the visibility & player counts
void CUnit::ShowWindow( )
{

    // if its not ours we can't look at it
#ifdef _CHEAT
    if ( !_bClickAny )
#endif
        if ( ( m_pOwner == NULL ) || ( !m_pOwner->IsMe( ) ) )
        {
            TRAP( );  // BUGBUG - what is this???
            CString sMsg;
            sMsg.LoadString( IDS_NOT_MY_UNIT );
            theApp.m_wndBar.SetStatusText( 0, sMsg, CStatInst::warn );
            MessageBeep( 0 );
            return;
        }

    // need to make the window?
    CWndArea* pWnd = theAreaList.BringToTop( );
    if ( pWnd == NULL )
    {
        pWnd = new CWndArea( );
        pWnd->Create( GetWorldPixels( ), this, FALSE );
        return;
    }

    // got it - center on the unit
    pWnd->Center( this );
}

void CUnit::MsgSetTarget( CUnit* pTarget )
{

    // can't shoot yourself
    if ( pTarget == this )
        return;

    if ( GetUnitType( ) == CUnit::vehicle )
        ( (CVehicle*)this )->TempTargetOff( );

    CMsgAttack msg( m_dwID, pTarget == NULL ? 0 : pTarget->GetID( ) );
    theGame.PostToClient( GetOwner( ), (CNetCmd*)&msg, sizeof( CMsgAttack ) );
}

void CUnit::_SetTarget( CUnit* pTarget )
{

    ASSERT_VALID( this );
    ASSERT_VALID_OR_NULL( pTarget );

    // can't shoot yourself
    if ( pTarget == this )
        return;

    if ( GetFireRate( ) == 0 )
        return;

    // init it
    m_pUnitTarget = pTarget;

    if ( pTarget == NULL )
    {
        if ( GetUnitType( ) == CUnit::vehicle )
            ( (CVehicle*)this )->TempTargetOff( );
        return;
    }

    ASSERT( ( pTarget->GetUnitType( ) != CUnit::vehicle ) || ( ( (CVehicle*)pTarget )->GetHexOwnership( ) == TRUE ) );

    m_iLOS = theMap.GetRangeDistance( this, m_pUnitTarget );
    if ( m_iLOS < GetRange( ) )
        m_iLOS = theMap.LineOfSight( this, m_pUnitTarget );

    if ( m_pUnitTarget->GetUnitType( ) == CUnit::vehicle )
        m_ptTarget = ( (CVehicle*)m_pUnitTarget )->GetPtHead( );
    else if ( m_pUnitTarget->GetUnitType( ) == CUnit::building )
    {
        CHexCoord _hex;
        if ( GetUnitType( ) == CUnit::building )
            _hex = ( (CBuilding*)this )->GetHex( );
        else if ( GetUnitType( ) == CUnit::vehicle )
            _hex = ( (CVehicle*)this )->GetPtHead( );

        CBuilding* pBldg = (CBuilding*)m_pUnitTarget;
        if ( _hex.X( ) < pBldg->GetHex( ).X( ) )
            _hex.X( ) = pBldg->GetHex( ).X( );
        else if ( _hex.X( ) > pBldg->GetHex( ).X( ) + pBldg->GetCX( ) )
            _hex.X( ) = pBldg->GetHex( ).X( ) + pBldg->GetCX( ) - 1;
        if ( _hex.Y( ) < pBldg->GetHex( ).Y( ) )
            _hex.Y( ) = pBldg->GetHex( ).Y( );
        else if ( _hex.Y( ) > pBldg->GetHex( ).Y( ) + pBldg->GetCY( ) )
            _hex.Y( ) = pBldg->GetHex( ).Y( ) + pBldg->GetCY( ) - 1;
        _hex.Wrap( );
        m_ptTarget = _hex;
    }

#ifdef _LOGOUT
    logPrintf( LOG_PRI_USEFUL, LOG_ATTACK, "Unit %d SetTarget (%d) LOS: %d", GetID( ), pTarget->GetID( ), m_iLOS );
#endif

    // set it up
    if ( GetUnitType( ) == vehicle )
    {
        CVehicle* pVeh = (CVehicle*)this;
        pVeh->SetEvent( CVehicle::attack );
        // force a recalculation
        m_ptUs.x = pVeh->GetPtHead( ).x + 20;
        m_ptUs.Wrap( );

        // get us moving if blocked
        if ( ( pVeh->GetRouteMode( ) != CVehicle::stop ) && ( pVeh->GetRouteMode( ) != CVehicle::moving ) )
            pVeh->SetDest( m_ptTarget );
    }
}

void CUnit::MsgSetTarget( CSubHex const& _sub )
{

    ASSERT_VALID( this );

    // lets find the target
    if ( ( m_pUnitTarget = theBuildingHex._GetBuilding( _sub ) ) == NULL )
        m_pUnitTarget = theVehicleHex._GetVehicle( _sub );

    // get the line of sight
    if ( m_pUnitTarget != NULL )
    {
        MsgSetTarget( m_pUnitTarget );
        return;
    }

    TRAP( );
    m_ptTarget = _sub;
    m_iLOS     = -2000;

    // set it up
    if ( GetUnitType( ) == vehicle )
    {
        ( (CVehicle*)this )->SetEvent( CVehicle::none );
        ( (CVehicle*)this )->SetDest( _sub );
    }
}

void CUnit::SetOppo( CUnit* pUnit )
{

    ASSERT_VALID( this );
    ASSERT_VALID_OR_NULL( pUnit );

    if ( ( GetFireRate( ) == 0 ) || ( !GetOwner( )->IsLocal( ) ) )
        return;

    // if no change - leave
    if ( ( pUnit == m_pUnitOppo ) && ( pUnit != NULL ) )
        return;

    // check for losing the target
    if ( ( GetOwner( )->IsAI( ) ) && ( m_pUnitOppo == m_pUnitTarget ) && ( m_pUnitTarget != NULL ) )
    {
        CMsgOutOfLos msg( this, m_pUnitTarget );
        theGame.PostToClient( GetOwner( ), &msg, sizeof( msg ) );
    }

#ifdef _LOGOUT
    logPrintf( LOG_PRI_USEFUL, LOG_ATTACK, "Unit %d SetOppo (%d)", GetID( ), pUnit == NULL ? 0 : pUnit->GetID( ) );
#endif

    if ( ( m_pUnitOppo = pUnit ) == NULL )
    {
        DetermineOppo( );
        return;
    }

    m_iOppoLOS = theMap.GetRangeDistance( this, m_pUnitOppo );
    if ( GetUnitType( ) == CUnit::vehicle )
        m_ptUsOppo = ( (CVehicle*)this )->GetPtHead( );
    if ( pUnit->GetUnitType( ) == CUnit::vehicle )
        m_ptOppo = ( (CVehicle*)pUnit )->GetPtHead( );

    // tell the target
    CMsgUnitAttacked msg( this, pUnit );
    theGame.PostToClient( pUnit->GetOwner( ), &msg, sizeof( msg ) );
}

int CUnit::GetTotalStore( ) const
{

    ASSERT_VALID( this );

    int iTotal = 0;
    for ( int iOn = 0; iOn < CMaterialTypes::GetNumTypes( ); iOn++ ) iTotal += GetStore( iOn );

    if ( GetUnitType( ) == CUnit::vehicle )
    {
        TRAP( iTotal > ( (CVehicle*)this )->GetData( )->GetMaxMaterials( ) );
        iTotal = __min( iTotal, ( (CVehicle*)this )->GetData( )->GetMaxMaterials( ) );
    }

    iTotal = __max( 0, iTotal );

    return ( iTotal );
}

//---------------------------------------------------------------------------
// GetDirectionIndex
//---------------------------------------------------------------------------
static int GetDirectionIndex( int iDir, CVehicleSprite::TILT_TYPE eTilt )
{
    if ( CVehicleSprite::TILT_BACK != eTilt )
        iDir = ( 48 - ToIsometricDir( iDir, eTilt ) ) & 0x007f;

    int iIndex = ( ( ( iDir + FULL_ROT / 16 ) * 8 ) / FULL_ROT - xiDir * 2 ) & 0x07;

    return iIndex;
}

/////////////////////////////////////////////////////////////////////////////
// CBuilding - a building

static int fnEnumIsLive( CHex* phex, CHexCoord, void* pData )
{

    if ( phex->GetVisible( ) != 0 )
    {
        *( (BOOL*)pData ) = TRUE;
        return TRUE;
    }

    return FALSE;
}

// returns TRUE if the building can be seen now
BOOL CBuilding::IsLive( ) const
{

    // if mine it's visible
    if ( GetOwner( )->IsMe( ) )
        return ( TRUE );

    // if not visible it's not live
    if ( !IsVisible( ) )
        return ( FALSE );

    // see if any hexes are visible
    BOOL bRtn = FALSE;
    theMap._EnumHexes( GetHex( ), GetCX( ), GetCY( ), fnEnumIsLive, &bRtn );

    return ( bRtn );
}

// lets find a hex to dump it out at
CHexCoord CBuilding::GetExitDest( CTransportData const* pTd, BOOL bFar )
{

    int     iTrys = 30;
    int     iDist = bFar ? 7 : 2;
    CSubHex _sub, _orig, _start, _ok;

    int iWheelType = pTd->GetWheelType( );
    int iSlowSpeed = 2 * theTerrain.GetData( CHex::plain ).GetWheelMult( iWheelType );

    _start = _orig = CSubHex( GetExit( iWheelType ) );
    switch ( ( pTd->GetWheelType( ) == CWheelTypes::water ) ? GetShipDir( ) : GetExitDir( ) )
    {
    case 0:
        _orig.y -= iDist;
        break;
    case 1:
        _orig.x += iDist + 1;
        break;
    case 2:
        _orig.y += iDist + 1;
        break;
    case 3:
        _orig.x -= iDist;
        break;
    }
    _ok = _orig;

    while ( iTrys-- )
    {
        _sub = _orig;

        // push it up to 4 away
        _sub.x += RandNum( iDist * 2 ) - iDist;
        _sub.y += RandNum( iDist * 2 ) - iDist;
        _sub.Wrap( );

        // can't be the exit hex
        if ( _sub.SameHex( _start ) )
        {
            iDist++;
            continue;
        }

        // not blocking another exit (brute force - not next to another building)
        CHexCoord _hex( _sub );
        if ( ( theBuildingHex.GetBuilding( _hex.X( ), _hex.Y( ) - 1 ) != NULL ) ||
             ( theBuildingHex.GetBuilding( _hex.X( ) - 1, _hex.Y( ) ) != NULL ) ||
             ( theBuildingHex.GetBuilding( _hex.X( ) + 1, _hex.Y( ) ) != NULL ) ||
             ( theBuildingHex.GetBuilding( _hex.X( ), _hex.Y( ) + 1 ) != NULL ) )
        {
            if ( _ok == _orig )
                _ok = _sub;
            iDist++;
            continue;
        }

        CHex* pHex = theMap._GetHex( _sub );

        // can we travel on it?
        int iSpeed = theTerrain.GetData( pHex->GetType( ) ).GetWheelMult( iWheelType );
        if ( iSpeed <= 0 )
            continue;

        // not on road (but road is better than exit for _ok)
        if ( pHex->GetType( ) == CHex::road )
        {
            if ( _ok == _orig )
                _ok = _sub;
            iDist++;
            continue;
        }

        // not too slow (but is better than exit)
        if ( iSpeed > iSlowSpeed )
        {
            if ( _ok == _orig )
                _ok = _sub;
            iDist++;
            continue;
        }

        // no unit there
        if ( theBuildingHex._GetBuilding( _sub ) == NULL )
            if ( pTd->CanEnterHex( _sub, _sub, FALSE, TRUE ) )
            {
                if ( theVehicleHex._GetVehicle( _sub ) == NULL )
                    return ( _sub );
                _ok = _sub;
            }

        iDist += 2;
    }

    // ok - never found it - use the last anyways (exit hex too dangerous)
    return ( _ok );
}

void CBuilding::DetermineOppo( )
{

    // do we care?
    if ( ( !GetOwner( )->IsLocal( ) ) || ( m_pUnitOppo != NULL ) || ( GetRange( ) <= 0 ) )
        return;

    ASSERT_VALID( this );

    int*      piOn = CVehicle::m_apiWid[GetRange( )];
    CHexCoord _hex( m_hex.X( ), m_hex.Y( ) - GetRange( ) );
    _hex.Wrap( );
    int    iDamageOppo = 0;
    CUnit* pOppo       = NULL;

    // oppo fire for human player
    if ( GetOwner( )->IsMe( ) )
    {
        int yMax = GetRange( ) + m_cy - 1;
        for ( int y = -GetRange( ); y < yMax; y++ )
        {
            _hex.X( m_hex.X( ) - *piOn );
            CHex* pHex = theMap._GetHex( _hex );

            for ( int x = ( *piOn ) * 2 + m_cx - 1; x >= 0; x-- )
            {
                if ( ( pHex->GetVisibility( ) ) &&
                     ( pHex->GetUnits( ) & ( CHex::ul | CHex::ur | CHex::ll | CHex::lr | CHex::bldg ) ) )
                {
                    if ( pHex->GetUnits( ) & CHex::bldg )
                        CheckOppo( theBuildingHex._GetBuilding( _hex ), iDamageOppo, &pOppo );
                    CSubHex _sub( _hex.X( ) * 2, _hex.Y( ) * 2 );
                    if ( pHex->GetUnits( ) & CHex::ul )
                        CheckOppo( theVehicleHex._GetVehicle( _sub ), iDamageOppo, &pOppo );
                    _sub.x++;
                    if ( pHex->GetUnits( ) & CHex::ur )
                        CheckOppo( theVehicleHex._GetVehicle( _sub ), iDamageOppo, &pOppo );
                    _sub.y++;
                    if ( pHex->GetUnits( ) & CHex::lr )
                        CheckOppo( theVehicleHex._GetVehicle( _sub ), iDamageOppo, &pOppo );
                    _sub.x--;
                    if ( pHex->GetUnits( ) & CHex::ll )
                        CheckOppo( theVehicleHex._GetVehicle( _sub ), iDamageOppo, &pOppo );
                    if ( ( m_pUnitOppo != NULL ) && ( iDamageOppo == 2 ) )
                    {
                        SetOppo( pOppo );
                        return;
                    }
                }

                _hex.Xinc( );
                pHex = theMap._Xinc( pHex );
            }
            _hex.Yinc( );
            piOn++;
        }

        // something is better than nothing
        if ( pOppo != NULL )
            SetOppo( pOppo );
        return;
    }

    // oppo fire for AI
    int yMax = GetRange( ) + m_cy - 1;
    for ( int y = -GetRange( ); y < yMax; y++ )
    {
        _hex.X( m_hex.X( ) - *piOn );
        CHex* pHex = theMap._GetHex( _hex );

        for ( int x = ( *piOn ) * 2 + m_cx - 1; x >= 0; x-- )
        {
            if ( pHex->GetUnits( ) & ( CHex::ul | CHex::ur | CHex::ll | CHex::lr | CHex::bldg ) )
            {
                if ( pHex->GetUnits( ) & CHex::bldg )
                    CheckAiOppo( theBuildingHex._GetBuilding( _hex ), &pOppo, FALSE );
                CSubHex _sub( _hex.X( ) * 2, _hex.Y( ) * 2 );
                if ( pHex->GetUnits( ) & CHex::ul )
                    CheckAiOppo( theVehicleHex._GetVehicle( _sub ), &pOppo, FALSE );
                _sub.x++;
                if ( pHex->GetUnits( ) & CHex::ur )
                    CheckAiOppo( theVehicleHex._GetVehicle( _sub ), &pOppo, FALSE );
                _sub.y++;
                if ( pHex->GetUnits( ) & CHex::lr )
                    CheckAiOppo( theVehicleHex._GetVehicle( _sub ), &pOppo, FALSE );
                _sub.x--;
                if ( pHex->GetUnits( ) & CHex::ll )
                    CheckAiOppo( theVehicleHex._GetVehicle( _sub ), &pOppo, FALSE );
                if ( pOppo != NULL )
                {
                    SetOppo( pOppo );
                    return;
                }
            }
            _hex.Xinc( );
            pHex = theMap._Xinc( pHex );
        }
        _hex.Yinc( );
        piOn++;
    }
}

CHexCoord                                // Return hexcoord of left corner, as viewed
CBuilding::GetLeftHex( int iDir ) const  // Current view direction
{
    ASSERT_VALID( this );

    CHexCoord hexcoord = GetHex( );

    switch ( iDir )
    {
    case 0:
        break;

    case 1:
        hexcoord.X( ) += GetCX( ) - 1;
        break;

    case 2:
        hexcoord.X( ) += GetCX( ) - 1;
        hexcoord.Y( ) += GetCY( ) - 1;
        break;

    case 3:
        hexcoord.Y( ) += GetCY( ) - 1;
        break;

    default:
        ASSERT( 0 );
    }

    return hexcoord;
}

CHexCoord                               // Return hexcoord of top corner, as viewed
CBuilding::GetTopHex( int iDir ) const  // Current view direction
{
    ASSERT_VALID( this );

    CHexCoord hexcoord = GetHex( );

    switch ( iDir )
    {
    case 3:
        break;

    case 0:
        hexcoord.X( ) += GetCX( ) - 1;
        break;

    case 1:
        hexcoord.X( ) += GetCX( ) - 1;
        hexcoord.Y( ) += GetCY( ) - 1;
        break;

    case 2:
        hexcoord.Y( ) += GetCY( ) - 1;
        break;

    default:
        ASSERT( 0 );
    }

    return hexcoord;
}

void CBuilding::UpdateConst( CMsgBldgStat* pMsg )
{

    ASSERT_VALID( this );

    if ( ( m_iConstDone != -1 ) && ( pMsg->m_iConstDone == -1 ) )
        GetOwner( )->AddExists( GetData( )->GetType( ), 1 );
    m_iLastPer   = pMsg->m_iBuilt;
    m_iConstDone = pMsg->m_iConstDone;
    m_iFoundPer  = pMsg->m_iFoundPer;
    m_iSkltnPer  = pMsg->m_iSkltnPer;
    m_iFinalPer  = pMsg->m_iFinalPer;

    // can we see it?
    if ( ( GetUnitType( ) != CUnit::building ) || IsLive( ) || IsFlag( CUnit::show_bldg ) )
        SetConstPer( );
}

void CBuilding::GetDesc( CString& sText ) const
{

    ASSERT_VALID( this );
    sText = m_pUnitData->GetDesc( );

#ifdef _CHEAT
    if ( _bShowStatus )
        sText += " ID:" + IntToCString( m_dwID ) + " Own:" + IntToCString( GetOwner( )->GetPlyrNum( ) ) +
                 " Dmg:" + IntToCString( m_iDamagePer ) +
                 " Trgt:" + ( m_pUnitTarget == NULL ? "0" : IntToCString( m_pUnitTarget->GetID( ) ) ) +
                 " Oppo:" + ( m_pUnitOppo == NULL ? "0" : IntToCString( m_pUnitOppo->GetID( ) ) );
#endif
}

//------------------------------------------------------------------------
// fnEnumFoundationTerrain: Draw terrain tiles
//------------------------------------------------------------------------
static int fnEnumFoundationTerrain( CHex* phex, CHexCoord hexcoord, void* )
{
    hexcoord.SetInvalidated( );

    CRect rectBound = phex->Draw( hexcoord );

    return FALSE;
}

//------------------------------------------------------------------------
// fnEnumInvalidateHex: Invalidate hexs
//------------------------------------------------------------------------
static int fnEnumInvalidateHex( CHex* phex, CHexCoord hexcoord, void* )
{
    hexcoord.Wrap( );

    return FALSE;
}

//---------------------------------------------------------------------------
// CBuilding::SetInvalidated
//---------------------------------------------------------------------------
void CBuilding::SetInvalidated( )
{
    theMap._EnumHexes( GetHex( ), GetCX( ), GetCY( ), fnEnumInvalidateHex, NULL );
}

//---------------------------------------------------------------------------
// CBuilding::DrawFoundation - Draw underlying terrain and foundation
//---------------------------------------------------------------------------
void CBuilding::DrawFoundation( const CHexCoord& hexcoord )
{
    // FIXIT: Assumes foundation is 1-part (seems reasonable though)

    // All foundation hexes are drawn, but only if this is the top hex

    CStructureData::BLDG_TYPE eType = GetData( )->GetType( );

    BOOL bFinished = -1 == m_iVisConstDone || -1 == m_iVisFinalPer || -1 == m_iVisSkltnPer;
    // BUGBUG	BOOL	bSeeTerrain = CStructureData::seaport 	  == eType ||
    //							  CStructureData::shipyard_1 == eType ||
    //							  CStructureData::shipyard_3 == eType;
    BOOL bSeeTerrain = CStructureData::seaport == eType || CStructureData::shipyard_1 == eType;

    if ( bFinished && !bSeeTerrain )
        return;
    auto point = CPoint( 0, 0 );
    CDrawParms drawparms( *this, point );
    CHexCoord  hexcoordLeft = GetLeftHex( xiDir );

    hexcoordLeft.ToNearestHex( hexcoord );

    CMapLoc maploc( hexcoordLeft );
    int     cx = GetCX( );
    int     cy = GetCY( );

    // Draw terrain under the foundation tiles, if foundation not finished

    CHexCoord hexcoordLeftMap = GetHex( );

    hexcoordLeftMap.ToNearestHex( hexcoord );

    if ( bSeeTerrain || ( -1 != m_iVisFoundPer && IsTopHex( hexcoord ) ) )
        theMap._EnumHexes( hexcoordLeftMap, cx, cy, fnEnumFoundationTerrain, NULL );
}

//---------------------------------------------------------------------------
// CBuilding::IsTopHex
//---------------------------------------------------------------------------
BOOL CBuilding::IsTopHex( const CHexCoord& hexcoord ) const
{
    CHexCoord hexcoordTop = GetTopHex( xiDir );

    hexcoordTop.ToNearestHex( hexcoord );

    return hexcoord == hexcoordTop;
}

//---------------------------------------------------------------------------
// CBuilding::GetDrawCorner
//---------------------------------------------------------------------------
CPoint CBuilding::GetDrawCorner( const CHexCoord& hexcoord ) const
{
    CPoint aptHex[4];
    int    cy;

    if ( 1 == xiDir || 3 == xiDir )
        cy = GetCX( );
    else
        cy = GetCY( );

    xpanimatr->MapToWindowHex( hexcoord, aptHex );

    ASSERT( 0 <= HEX_HT_PWR - 1 - xiZoom );

    int iX = aptHex[0].x;
    int iY = aptHex[0].y + ( cy << HEX_HT_PWR - 1 - xiZoom );

    return CPoint( iX, iY );
}

//---------------------------------------------------------------------------
// CBuilding::Draw
//---------------------------------------------------------------------------
CRect CBuilding::Draw( const CHexCoord& hexcoord )
{
    ASSERT_STRICT_VALID( this );

    auto point = CPoint( 0, 0 );
    CDrawParms drawparms( *this, point );
    CPoint     ptCorner = GetDrawCorner( hexcoord );

    CPoint       ptOffset = ptCorner;
    CMapLoc      maploc( hexcoord );
    int          iDir = GetDrawDir( );
    CSpriteView* pspriteviewBldg;
    CRect        rect;

    ASSERT( -1 <= m_iVisFinalPer && m_iVisFinalPer <= 100 );
    ASSERT( -1 <= m_iVisSkltnPer && m_iVisSkltnPer <= 100 );
    ASSERT( -1 <= m_iVisFinalPer && m_iVisFinalPer <= 100 );

    // if built then its a normal draw
    if ( m_iVisConstDone == -1 || m_iVisFinalPer == -1 )
    {
        pspriteviewBldg = GetSprite( )->GetView( iDir, GetLayer( ), CStructureSprite::COMPLETED_STAGE, m_bDamage );

        ptOffset.y = ptCorner.y - pspriteviewBldg->Height( );

        drawparms.m_ptOffset = ptOffset;

        rect = pspriteviewBldg->Draw( drawparms );

        // Turret

        if ( GetTurret( ) )
        {
            CHotSpotKey     hotspotkey( CHotSpotKey::TURRET_HOTSPOT, 0 );
            CHotSpot const* photspotTurret = pspriteviewBldg->GetHotSpot( hotspotkey );

            ASSERT( photspotTurret );

            if ( photspotTurret )
            {
                CPoint ptHotSpotClient = ptOffset + pspriteviewBldg->GetHotSpotPoint( *photspotTurret );

                rect |= DrawTurret( ptOffset );
            }
        }

        // Flag

        if ( GetFlag( ) )
        {
            CHotSpotKey     hotspotkey( CHotSpotKey::FLAG_HOTSPOT, 0 );
            CHotSpot const* photspotFlag = pspriteviewBldg->GetHotSpot( hotspotkey );

            // ASSERT( photspotFlag || IsTwoPiece() ); GGTODO: uncomment when art fixed

            if ( photspotFlag )
            {
                CPoint ptHotSpotClient = ptOffset + pspriteviewBldg->GetHotSpotPoint( *photspotFlag );

                GetFlag( )->Draw( ptHotSpotClient );
            }
        }

        // Smoke/Flame

        if ( GetDamageDisplay( ) )
            GetDamageDisplay( )->Draw( *pspriteviewBldg, drawparms );  // Self-invalidating

        if ( CDrawParms::IsInvalidateMode( ) && IsInvalidated( ) )
            xpanimatr->GetDirtyRects( )->AddRect( &rect );

        return rect;
    }

    if ( CSprite::BACKGROUND_LAYER == GetLayer( ) || !IsTwoPiece( ) )
    {
        // Get the foundation sprite

        pspriteviewBldg =
            GetSprite( )->GetView( iDir, CSprite::FOREGROUND_LAYER, CStructureSprite::FOUNDATION_STAGE, m_bDamage );

        ptOffset.y           = ptCorner.y - pspriteviewBldg->Height( );
        drawparms.m_ptOffset = ptOffset;

        // If foundation not complete, draw partial foundation

        if ( m_iVisFoundPer != -1 )
        {
            rect = pspriteviewBldg->Rect( ) - pspriteviewBldg->Rect( ).TopLeft( ) + drawparms.m_ptOffset;

            rect.top += ( ( 100 - m_iVisFoundPer ) * rect.Height( ) ) / 100;

            CRect rectBound = rect & pspriteviewBldg->DrawClip( drawparms, &rect );

            if ( CDrawParms::IsInvalidateMode( ) )
                xpanimatr->GetDirtyRects( )->AddRect( &rectBound );

            return rectBound;
        }

        // Draw completed foundation

        pspriteviewBldg->Draw( drawparms );

        // Skeleton

        pspriteviewBldg =
            GetSprite( )->GetView( iDir, CSprite::FOREGROUND_LAYER, CStructureSprite::SKELETON_STAGE, m_bDamage );

        ptOffset.y = ptCorner.y - pspriteviewBldg->Height( );

        drawparms.m_ptOffset = ptOffset;

        // Draw partial skeleton
        if ( m_iVisSkltnPer != -1 )
        {
            rect = pspriteviewBldg->Rect( ) - pspriteviewBldg->Rect( ).TopLeft( ) + drawparms.m_ptOffset;

            rect.top += ( ( 100 - m_iVisSkltnPer ) * rect.Height( ) ) / 100;

            CRect rectBound = rect & pspriteviewBldg->DrawClip( drawparms, &rect );

            if ( CDrawParms::IsInvalidateMode( ) )
                xpanimatr->GetDirtyRects( )->AddRect( &rectBound );

            // Smoke/Flame

            if ( GetDamageDisplay( ) )
                GetDamageDisplay( )->Draw( *pspriteviewBldg, drawparms );

            return rectBound;
        }

        // Draw completed skeleton
        pspriteviewBldg->Draw( drawparms );
    }

    // Draw partial construction
    pspriteviewBldg = GetSprite( )->GetView( iDir, GetLayer( ), CStructureSprite::CONSTRUCTION_STAGE, m_bDamage );

    ptOffset.y = ptCorner.y - pspriteviewBldg->Height( );
    rect       = pspriteviewBldg->Rect( ) - pspriteviewBldg->Rect( ).TopLeft( ) + ptOffset;
    rect.top += ( ( 100 - m_iVisFinalPer ) * rect.Height( ) ) / 100;

    drawparms.m_ptOffset = ptOffset;

    CRect rectBound = rect & pspriteviewBldg->DrawClip( drawparms, &rect );

    if ( CDrawParms::IsInvalidateMode( ) )
        xpanimatr->GetDirtyRects( )->AddRect( &rectBound );

    // Smoke/Flame

    if ( GetDamageDisplay( ) )
        GetDamageDisplay( )->Draw( *pspriteviewBldg, drawparms );

    return rectBound;
}

//---------------------------------------------------------------------------
// CBuilding::DrawTurret
//---------------------------------------------------------------------------
CRect CBuilding::DrawTurret( CPoint ptHotSpotClient )
{
    CRect rect( 0, 0, 0, 0 );

    if ( !GetTurret( ) )
        return rect;

    int iTiltIndex      = CVehicleSprite::TILT_FLAT;
    int iDirectionIndex = GetDirectionIndex( GetTurret( )->GetDir( ), CVehicleSprite::TILT_TYPE( iTiltIndex ) );

    CSpriteView* pspriteviewTurret = GetTurret( )->GetSprite( )->GetView( iDirectionIndex, iTiltIndex, m_bDamage );

    CPoint ptAnchor = pspriteviewTurret->GetAnchorPoint( );
    CPoint ptOffset =
        CPoint( ( pspriteviewTurret->Width( ) >> 1 ) - ptAnchor.x, ( pspriteviewTurret->Height( ) >> 1 ) - ptAnchor.y );

    CQuadDrawParms drawparms( *GetTurret( ), ptOffset, ptHotSpotClient, 0 );

    AdjustDirection( drawparms, iTiltIndex, iDirectionIndex, GetTurret( )->GetDir( ) );

    rect = pspriteviewTurret->Draw( drawparms );

    return rect;
}

//---------------------------------------------------------------------------
// CBuilding::IsTwoPiece
//---------------------------------------------------------------------------
BOOL CBuilding::IsTwoPiece( ) const
{
    CStructureSprite::STAGE_TYPE eStage;

    if ( -1 == m_iVisConstDone || -1 == m_iVisFinalPer )
        eStage = CStructureSprite::COMPLETED_STAGE;
    else if ( -1 == m_iVisSkltnPer )
        eStage = CStructureSprite::CONSTRUCTION_STAGE;
    else if ( -1 == m_iVisFoundPer )
        eStage = CStructureSprite::SKELETON_STAGE;
    else
        eStage = CStructureSprite::FOUNDATION_STAGE;

    return GetSprite( )->IsTwoPiece( GetDrawDir( ), eStage, m_bDamage );
}

//---------------------------------------------------------------------------
// CBuilding::IsHit
//---------------------------------------------------------------------------
BOOL CBuilding::IsHit( CHexCoord hexcoord, CPoint ptCursor ) const
{
    ASSERT_STRICT_VALID( this );

    CPoint     ptCorner = GetDrawCorner( hexcoord );
    CPoint     ptOffset = ptCorner;
    auto point = CPoint( 0, 0 );
    CDrawParms drawparms( (CBuilding&)*this, point );

    CSpriteView* pspriteview;
    int          iDir = GetDrawDir( );

    if ( m_iVisConstDone == -1 || m_iFinalPer == -1 )
    {
        pspriteview = GetSprite( )->GetView( iDir, CStructureSprite::FOREGROUND_LAYER,
                                             CStructureSprite::COMPLETED_STAGE, m_bDamage );

        ptOffset.y = ptCorner.y - pspriteview->Height( );

        drawparms.m_ptOffset = ptOffset;

        if ( pspriteview->IsHit( drawparms, ptCursor ) )
            return TRUE;

        if ( GetSprite( )->IsTwoPiece( iDir, CStructureSprite::COMPLETED_STAGE, m_bDamage ) )
        {
            pspriteview = GetSprite( )->GetView( iDir, CStructureSprite::BACKGROUND_LAYER,
                                                 CStructureSprite::COMPLETED_STAGE, m_bDamage );

            ptOffset.y = ptCorner.y - pspriteview->Height( );

            drawparms.m_ptOffset = ptOffset;

            if ( pspriteview->IsHit( drawparms, ptCursor ) )
                return TRUE;
        }

        if ( GetDamageDisplay( ) )
            return GetDamageDisplay( )->IsHit( *pspriteview, drawparms, ptCursor );

        return FALSE;
    }

    // GGFIXIT: May need to check both parts of 2-part structures?

    // Foundation

    pspriteview = GetSprite( )->GetView( iDir, CStructureSprite::FOREGROUND_LAYER, CStructureSprite::FOUNDATION_STAGE,
                                         m_bDamage );

    ptOffset.y = ptCorner.y - pspriteview->Height( );

    drawparms.m_ptOffset = ptOffset;

    CRect rectClip = pspriteview->Rect( ) - pspriteview->Rect( ).TopLeft( ) + drawparms.m_ptOffset;

    if ( m_iVisFoundPer != -1 )
        rectClip.top += ( ( 100 - m_iVisFoundPer ) * rectClip.Height( ) ) / 100;

    if ( pspriteview->IsHitClip( drawparms, ptCursor, &rectClip ) )
        return TRUE;

    if ( m_iVisFoundPer != -1 )
        return FALSE;

    // Skeleton

    pspriteview =
        GetSprite( )->GetView( iDir, CStructureSprite::FOREGROUND_LAYER, CStructureSprite::SKELETON_STAGE, m_bDamage );

    ptOffset.y = ptCorner.y - pspriteview->Height( );

    drawparms.m_ptOffset = ptOffset;

    rectClip = pspriteview->Rect( ) - pspriteview->Rect( ).TopLeft( ) + drawparms.m_ptOffset;

    if ( m_iVisSkltnPer != -1 )
        rectClip.top += ( ( 100 - m_iVisSkltnPer ) * rectClip.Height( ) ) / 100;

    if ( pspriteview->IsHitClip( drawparms, ptCursor, &rectClip ) )
        return TRUE;

    if ( m_iVisSkltnPer != -1 )
        return FALSE;

    // Construction

    pspriteview = GetSprite( )->GetView( iDir, CStructureSprite::FOREGROUND_LAYER, CStructureSprite::CONSTRUCTION_STAGE,
                                         m_bDamage );

    ptOffset.y = ptCorner.y - pspriteview->Height( );

    drawparms.m_ptOffset = ptOffset;

    rectClip = pspriteview->Rect( ) - pspriteview->Rect( ).TopLeft( ) + drawparms.m_ptOffset;

    if ( m_iVisFinalPer != -1 )
        rectClip.top += ( ( 100 - m_iVisFinalPer ) * rectClip.Height( ) ) / 100;

    return pspriteview->IsHitClip( drawparms, ptCursor, &rectClip );
}

void CBuilding::InvalidateStatus( ) const
{

    ASSERT_VALID( this );

    if ( ( GetOwner( ) == NULL ) || ( !GetOwner( )->IsMe( ) ) )
        return;

    // invalidate any area windows
    theAreaList.InvalidateStatus( this );

    // invalidate the list box
    int iIndex = theApp.m_wndBldgs.FindItem( this );
    if ( iIndex < 0 )
        return;
    int iTop = theApp.m_wndBldgs.m_ListBox.GetTopIndex( );
    if ( iIndex < iTop )
        return;

    CRect rect( 0, 0, 0, 0 );
    theApp.m_wndBldgs.m_ListBox.GetClientRect( &rect );
    rect.top = ( iIndex - iTop ) * 64;
    if ( rect.top >= rect.bottom )
        return;
    rect.bottom = __min( rect.bottom, rect.top + 64 );
    theApp.m_wndBldgs.m_ListBox.InvalidateRect( &rect, FALSE );
}

int CBuilding::GetBuiltPer( ) const
{

    ASSERT_VALID( this );

    // if done return 100%
    if ( m_iConstDone == -1 )
        return ( 100 );

    int iRtn = ( m_iConstDone * 100 ) / ( m_iFoundTime + GetData( )->GetTimeBuild( ) );
    ASSERT( 0 <= iRtn );

    // return the percentage
    if ( iRtn < 0 )
        return ( 0 );
    if ( iRtn > 100 )
        return ( 100 );
    return ( iRtn );
}

// materials needed by this building for itself (const & repair) - materials on hand
int CBuilding::GetBldgMatReq( int iInd, BOOL bSubStore ) const
{

    if ( iInd >= CMaterialTypes::GetNumBuildTypes( ) )
        return ( 0 );

    int iTotal = 0;
    if ( m_iConstDone != -1 )
        iTotal = NeedToBuild( iInd, 100 );

    iTotal += GetBldgMatRepair( iInd );

    if ( bSubStore )
        iTotal -= GetStore( iInd );
    return ( __max( 0, iTotal ) );
}

// materials needed to repair - doesn't look at materials on hand
int CBuilding::GetBldgMatRepair( int iInd ) const
{

    if ( ( m_iDamagePoints >= GetData( )->GetDamagePoints( ) ) || ( GetData( )->GetBuild( iInd ) <= 0 ) )
        return ( 0 );

    int iTotal =
        NeedToRepair( iInd, ( ( GetData( )->GetDamagePoints( ) - m_iDamagePoints ) * GetData( )->GetTimeBuild( ) ) /
                                ( 2 * GetData( )->GetDamagePoints( ) ) ) +
        1;

    return ( __max( 0, iTotal ) );
}

// this is done locally
void CBuilding::AddConstDone( int iDone )
{

    ASSERT_VALID( this );

    if ( !GetOwner( )->IsLocal( ) )
        return;

    // we are out of materials
    if ( m_unitFlags & event )
        return;

    // can we repair?
    if ( GetDamagePoints( ) < GetData( )->GetDamagePoints( ) )
    {
        if ( !IsFlag( repair_stop ) )
        {
            for ( int iInd = 0; iInd < CMaterialTypes::num_build_types; iInd++ ) m_aiRepair[iInd] += iDone;
            m_iRepairWork += iDone;
        }
        return;
    }

    if ( m_iRepairMod != 0 )
    {
        m_iRepairWork = 0;
        m_iRepairMod  = 0;
        memset( &m_aiRepair[0], 0, sizeof( m_aiRepair ) );
    }

    // are we done?
    if ( m_iConstDone == -1 )
    {
        if ( GetOwner( )->IsMe( ) )
            theGame.Event( EVENT_REPAIR_DONE, EVENT_NOTIFY, this );
        CVehicle::StopConstruction( this );
        return;
    }

    m_iConstDone += iDone;
}

void CBuilding::GetInputs( int* pVals ) const
{

    ASSERT_VALID( this );
    memset( pVals, 0, sizeof( int ) * CMaterialTypes::GetNumTypes( ) );
}

void CBuilding::GetAccepts( int* pVals ) const
{

    ASSERT_VALID( this );
    memset( pVals, 0, sizeof( int ) * CMaterialTypes::GetNumTypes( ) );
}

CStructureData::BLDG_TYPE CStructureData::GetBldgType( ) const
{

    ASSERT_VALID( this );

    switch ( m_iType )
    {
    case apartment_1_1:
    case apartment_1_2:
    case apartment_2_1:
    case apartment_2_2:
    case apartment_2_3:
    case apartment_2_4:
    case apartment_3_1:
    case apartment_3_2:
        return ( apartment );
    case office_2_1:
    case office_2_2:
    case office_2_3:
    case office_2_4:
    case office_3_1:
    case office_3_2:
        return ( office );
    case barracks_2:
    case barracks_3:
        return ( barracks );
    case fort_1:
    case fort_2:
    case fort_3:
        return ( fort );
    case light_0:
    case light_1:
    case light_2:
        return ( light );
    case power_1:
    case power_2:
    case power_3:
        return ( power );
    case shipyard_1:
    case shipyard_3:
        return ( shipyard );
    case rocket:
    case warehouse:
        return ( warehouse );
    }

    // rest are ok
    return ( m_iType );
}

/////////////////////////////////////////////////////////////////////////////
// CMaterialBuilding - builds materials from materials

void CMaterialBuilding::GetInputs( int* pVals ) const
{

    ASSERT_VALID( this );

    CBuildMaterials* pBldMtrls = GetData( )->GetBldMaterials( );
    for ( int iOn = 0; iOn < CMaterialTypes::GetNumTypes( ); iOn++ ) *pVals++ = pBldMtrls->GetInput( iOn );
}

int CMaterialBuilding::GetNextMinuteMat( int iInd ) const
{

    int iRtn = GetBldgResReq( iInd, FALSE );

    CBuildMaterials const* pBm = GetData( )->GetBldMaterials( );
    if ( pBm->GetInput( iInd ) == 0 )
        return ( iRtn );

    // add 1 minute of materials
    int iNum = ( 60 / pBm->GetTime( ) + 1 ) * pBm->GetInput( iInd );

    return ( iRtn + iNum );
}


/////////////////////////////////////////////////////////////////////////////
// CVehicleBuilding - builds vehicles

void CVehicleBuilding::GetInputs( int* pVals ) const
{

    ASSERT_VALID( this );
    memset( pVals, 0, sizeof( int ) * CMaterialTypes::GetNumTypes( ) );

    CBuildVehicle* pBldVeh = GetData( )->GetBldVehicle( );
    for ( int iOn = 0; iOn < CMaterialTypes::GetNumBuildTypes( ); iOn++ ) *pVals++ = pBldVeh->GetTotalInput( iOn );
}

int CVehicleBuilding::NeedToFinish( int iInd ) const
{

    ASSERT_VALID( this );

    if ( m_pBldUnt == NULL )
        return ( 0 );

    return ( m_pBldUnt->GetInput( iInd ) - m_aiUsed[iInd] );
}

int CVehicleBuilding::GetBldgResReq( int iInd, BOOL bAll ) const
{

    ASSERT_VALID( this );

    int iRtn = NeedToBuild( iInd, 100 ) + GetBldgMatRepair( iInd ) + NeedToFinish( iInd ) - GetStore( iInd );
    if ( ( !bAll ) || ( m_iNum <= 0 ) )
        return ( __max( 0, iRtn ) );

    // add in the additional vehicles needed
    iRtn += m_iNum * m_pBldUnt->GetInput( iInd );
    return ( __max( 0, iRtn ) );
}

int CVehicleBuilding::GetNextMinuteMat( int iInd ) const
{

    int iRtn = NeedToBuild( iInd, 100 ) + GetBldgMatRepair( iInd ) + NeedToFinish( iInd ) - GetStore( iInd );
    // this is it if nothing to build
    if ( ( m_iNum <= 0 ) || ( m_pBldUnt == NULL ) )
        return ( __max( 0, iRtn ) );

    // add in the additional vehicles needed over next 3 minutes
    int iNum3Min = ( 24 * 180 + m_pBldUnt->GetTime( ) - 1 ) / m_pBldUnt->GetTime( );
    iRtn += __min( iNum3Min, m_iNum ) * m_pBldUnt->GetInput( iInd );
    return ( __max( 0, iRtn ) );
}

/////////////////////////////////////////////////////////////////////////////
// CRepairBuilding - repairs vehicles

int CRepairBuilding::GetBldgResReq( int iInd, BOOL bAll ) const
{

    ASSERT_VALID( this );

    int iRtn = NeedToBuild( iInd, 100 ) + GetBldgMatRepair( iInd ) - GetStore( iInd );

    if ( m_pBldUnt == NULL )
        return ( __max( 0, iRtn ) );

    // add in the vehicles needed to be repaired
    if ( m_pVehRepairing != NULL )
    {
        int iTotal = m_pBldUnt->GetInput( iInd );
        iRtn +=
            iTotal - ( iTotal * m_pVehRepairing->GetDamagePoints( ) ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
    }

    if ( !bAll )
        return ( __max( 0, iRtn ) );
    TRAP( );

    CBuildUnit const* pBu = m_pBldUnt;

    POSITION pos = m_lstNext.GetHeadPosition( );
    while ( pos != NULL )
    {
        CVehicle* pVeh = m_lstNext.GetNext( pos );
        ( (CRepairBuilding*)this )->AssignBldUnit( m_pVehRepairing->GetData( )->GetType( ) );
        int iTotal = m_pBldUnt->GetInput( iInd );
        iRtn += iTotal - ( iTotal * pVeh->GetDamagePoints( ) ) / pVeh->GetData( )->GetDamagePoints( );
    }

    ( (CRepairBuilding*)this )->m_pBldUnt = pBu;

    return ( __max( 0, iRtn ) );
}


/////////////////////////////////////////////////////////////////////////////
// CShipyardBuilding - builds & repairs vehicles

int CShipyardBuilding::GetBldgResReq( int iInd, BOOL bAll ) const
{

    ASSERT_VALID( this );

    if ( m_iMode != repair )
        return CVehicleBuilding::GetBldgResReq( iInd, bAll );

    int iRtn = GetBldgMatRepair( iInd ) - GetStore( iInd );

    // add in the vehicles needed to be repaired
    if ( m_pVehRepairing != NULL )
    {
        int iTotal = m_pBldUnt->GetInput( iInd );
        iRtn +=
            iTotal - ( iTotal * m_pVehRepairing->GetDamagePoints( ) ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
    }

    if ( !bAll )
        return ( __max( 0, iRtn ) );
    TRAP( );

    CBuildUnit const* pBu = m_pBldUnt;

    POSITION pos = m_lstNext.GetHeadPosition( );
    while ( pos != NULL )
    {
        CVehicle* pVeh = m_lstNext.GetNext( pos );
        ( (CShipyardBuilding*)this )->AssignBldUnit( m_pVehRepairing->GetData( )->GetType( ) );
        int iTotal = m_pBldUnt->GetInput( iInd );
        iRtn += iTotal - ( iTotal * pVeh->GetDamagePoints( ) ) / pVeh->GetData( )->GetDamagePoints( );
    }

    ( (CShipyardBuilding*)this )->m_pBldUnt = pBu;

    return ( __max( 0, iRtn ) );
}


/////////////////////////////////////////////////////////////////////////////
// CPowerBuilding - creates power

void CPowerBuilding::GetInputs( int* pVals ) const
{

    ASSERT_VALID( this );
    memset( pVals, 0, sizeof( int ) * CMaterialTypes::GetNumTypes( ) );

    CBuildPower* pBldPwr = GetData( )->GetBldPower( );
    if ( pBldPwr->GetInput( ) >= 0 )
        pVals[pBldPwr->GetInput( )] = 1;
}

int CPowerBuilding::GetNextMinuteMat( int iInd ) const
{

    int iRtn = GetBldgResReq( iInd, FALSE );

    CBuildPower* pBp = GetData( )->GetBldPower( );
    if ( pBp->GetInput( ) != iInd )
        return ( iRtn );

    // add 1 minute of materials
    int iNum = 60 / pBp->GetRate( ) + 1;

    return ( iRtn + iNum );
}


/////////////////////////////////////////////////////////////////////////////
// CWarehouseBuilding - stores all materials

void CWarehouseBuilding::GetAccepts( int* pVals ) const
{

    ASSERT_VALID( this );

    for ( int iOn = 0; iOn < CMaterialTypes::GetNumTypes( ); iOn++ )
        if ( ( iOn != CMaterialTypes::food ) && ( iOn != CMaterialTypes::gas ) && ( iOn != CMaterialTypes::moly ) &&
             ( iOn != CMaterialTypes::goods ) )
            *pVals++ = 1;
        else
            *pVals++ = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CTurret - a turret

//--------------------------------------------------------------------------
// CTurret::DoMuzzleFlash - Play one-shot muzzle flash
//--------------------------------------------------------------------------
void CTurret::DoMuzzleFlash( )
{
    // Replay one-shot muzzle flash

    m_muzzleflash.SetOneShotAnimations( );  // Also resets them
    m_muzzleflash.EnableAnimations( );
}

/////////////////////////////////////////////////////////////////////////////
// CVehicle - a vehicle

void CVehicle::GetDesc( CString& sText ) const
{

    ASSERT_VALID( this );
    sText = m_pUnitData->GetDesc( );

#ifdef _CHEAT
    if ( _bShowStatus )
        sText += " ID:" + IntToCString( m_dwID ) + " Own:" + IntToCString( GetOwner( )->GetPlyrNum( ) ) +
                 " Dmg:" + IntToCString( m_iDamagePer ) +
                 " Trgt:" + ( m_pUnitTarget == NULL ? "0" : IntToCString( m_pUnitTarget->GetID( ) ) ) +
                 " Oppo:" + ( m_pUnitOppo == NULL ? "0" : IntToCString( m_pUnitOppo->GetID( ) ) );
#endif
}

//---------------------------------------------------------------------------
// CVehicle::IsInBuilding
//---------------------------------------------------------------------------
BOOL CVehicle::IsInBuilding( ) const
{
    CHexCoord hexcoord;
    CHex*     phex    = NULL;
    BYTE      byUnits = 0;

    hexcoord = GetPtHead( );
    phex     = theMap.GetHex( hexcoord );
    byUnits  = phex->GetUnits( );

    if ( byUnits & CHex::bldg )
        return TRUE;

    hexcoord = GetPtTail( );
    phex     = theMap.GetHex( hexcoord );
    byUnits  = phex->GetUnits( );

    if ( byUnits & CHex::bldg )
        return TRUE;

    return FALSE;
}

//---------------------------------------------------------------------------
// CVehicle::GetTiltIndex
//---------------------------------------------------------------------------
int CVehicle::GetTiltIndex( BOOL bOnBridge ) const
{
    BOOL bUseRollViews = TRUE;

    int iTilt = CVehicleSprite::TILT_FLAT;

    if ( !bOnBridge && bUseRollViews )
    {
        CPoint aptHex[4];

        xpanimatr->MapToWindowHex( GetWorldPixels( ), aptHex );

        int iHt   = CGameMap::HexHt( xiZoom );
        int iDel  = iHt / 3;  // This is tweakable
        int iDelY = aptHex[3].y - aptHex[1].y;

        if ( iDelY > iHt + iDel )
            iTilt = CVehicleSprite::TILT_FORWARD;
        else if ( iDelY < iHt - iDel )
            iTilt = CVehicleSprite::TILT_BACK;
    }

    return iTilt;
}

//---------------------------------------------------------------------------
// CFlag::Draw
//---------------------------------------------------------------------------
CRect CFlag::Draw( CPoint const& ptHotSpotClient )  // hot spot in window client coords
{
    CSpriteView* pspriteview = GetView( );

#ifdef _DEBUG
    pspriteview->CheckValid( );
#endif

    CPoint     ptAnchor = pspriteview->GetAnchorPoint( );
    CPoint     ptOffset = ptHotSpotClient - ptAnchor;
    CDrawParms drawparms( *this, ptOffset );

    CRect rectBound = pspriteview->Draw( drawparms );

    return rectBound;
}

//---------------------------------------------------------------------------
// CVehicle::GetDrawParms
//---------------------------------------------------------------------------
BOOL CVehicle::GetDrawParms( CQuadDrawParms& drawparms, CBridgeUnit const* pbridge ) const
{
    CMapLoc   maploc = GetMapLoc( );
    CMapLoc3D maploc3d( maploc.x, maploc.y, 0 );

    // Use bridge height if on a bridge, else use terrain height

    if ( pbridge && !IsOnWater( ) )
        maploc3d.m_fixZ = pbridge->GetSurfaceAlt( maploc );
    else
        maploc3d.m_fixZ = maploc.CalcAltitude( );

    CPoint ptCenter( xpanimatr->WorldToWindow( xpanimatr->WorldToCenterWorld( maploc3d ) ) );

    int iDY = 0;

    // If not on a bridge, adjust angle to match terrain

    if ( !pbridge )
    {
        CPoint aptHex[4];

        if ( !xpanimatr->MapToWindowHex( CHexCoord( GetMapLoc( ) ), aptHex ) )
            return FALSE;

        if ( ptCenter.x < aptHex[1].x )
            iDY = aptHex[0].y - ( (aptHex[1].y + aptHex[3].y) >> 1 ); // NOTE: are these parens correct?
        else
            iDY = -aptHex[2].y + ( (aptHex[1].y + aptHex[3].y) >> 1 );
    }

    int iDX = CGameMap::HexWid( xiZoom ) >> 1;

    BYTE byRotAngle = (BYTE)( asin( double( iDY ) / double( iDX ) ) * 256. / ( 2 * 3.14159265 ) );

    auto point = CPoint( 0, 0 );
    drawparms = CQuadDrawParms( (CVehicle&)*this, point , ptCenter, byRotAngle );

    return TRUE;
}

//---------------------------------------------------------------------------
// CVehicle::GetHotSpotClient
//---------------------------------------------------------------------------
BOOL  // TRUE if there is a hotspot
CVehicle::GetHotSpotClient( CSpriteView const& spriteviewVehicle, CQuadDrawParms const& drawparmsVehicle,
                            CHotSpotKey::HOTSPOT_TYPE eHotSpotType, int iHotSpotIndex,
                            CPoint* pptHotSpotClient )  // Output: Location of hotspot in window client coords
    const
{
    CHotSpotKey     hotspotkey( eHotSpotType, iHotSpotIndex );
    CHotSpot const* photspot = spriteviewVehicle.GetHotSpot( hotspotkey );

    if ( !photspot )
        return FALSE;

    // Get hotspot in client coords

    *pptHotSpotClient = spriteviewVehicle.GetHotSpotPoint( *photspot );
    *pptHotSpotClient = drawparmsVehicle.SpriteViewToWindow( *pptHotSpotClient, spriteviewVehicle );

    return TRUE;
}

//---------------------------------------------------------------------------
// CVehicle::GetBridgeOn - Return pointer to the bridge the vehicle is on, or NULL
//---------------------------------------------------------------------------
CBridgeUnit* CVehicle::GetBridgeOn( ) const
{
    CHexCoord hexcoordVehicle = GetMapLoc( );
    CHex*     phex            = theMap.GetHex( hexcoordVehicle );

    if ( phex->GetUnits( ) & CHex::bridge && !IsOnWater( ) )
        return theBridgeHex.GetBridge( hexcoordVehicle );

    return NULL;
}

//---------------------------------------------------------------------------
// CVehicle::Draw
//---------------------------------------------------------------------------
CRect CVehicle::Draw( const CHexCoord& )
{
    ASSERT_STRICT_VALID( this );

    CBridgeUnit*   pbridge = GetBridgeOn( );
    CQuadDrawParms drawparmsVehicle;

    int iTiltIndex      = GetTiltIndex( pbridge != NULL );
    int iDirectionIndex = GetDirectionIndex( m_iDir, CVehicleSprite::TILT_TYPE( iTiltIndex ) );

    if ( !GetDrawParms( drawparmsVehicle, pbridge ) )
        return CRect( 0, 0, 0, 0 );

    CQuadDrawParms drawparmsTurret = drawparmsVehicle;

    AdjustDirection( drawparmsVehicle, iTiltIndex, iDirectionIndex, m_iDir );

    CSpriteView* pspriteviewTurret  = NULL;
    CSpriteView* pspriteviewVehicle = GetSprite( )->GetView( iDirectionIndex, iTiltIndex, m_bDamage );

    CRect rectVehicle;

    rectVehicle = pspriteviewVehicle->Draw( drawparmsVehicle );

    CRect rectBound  = rectVehicle;
    CRect rectTurret = CRect( 0, 0, 0, 0 );

    // Draw turret

    if ( GetTurret( ) )
    {
        drawparmsTurret.m_ptile = GetTurret( );

        iDirectionIndex = GetDirectionIndex( GetTurret( )->GetDir( ), CVehicleSprite::TILT_TYPE( iTiltIndex ) );

        AdjustDirection( drawparmsTurret, iTiltIndex, iDirectionIndex, GetTurret( )->GetDir( ) );

        pspriteviewTurret = GetTurret( )->GetSprite( )->GetView( iDirectionIndex, iTiltIndex, m_bDamage );

        CHotSpotKey     hotspotkeyTurret( CHotSpotKey::TURRET_HOTSPOT, 0 );
        CHotSpot const* photspotTurret = pspriteviewVehicle->GetHotSpot( hotspotkeyTurret );

        // ASSERT( photspotTurret ); GGTODO - uncomment when corrected art available

        if ( photspotTurret )
        {
            // Place turret center over turret hotspot, then offset to align anchor with hotspot

            CPoint ptAnchorTurret        = pspriteviewTurret->GetAnchorPoint( );
            CPoint ptHotSpotTurret       = pspriteviewVehicle->GetHotSpotPoint( *photspotTurret );
            CPoint ptHotSpotClientTurret = drawparmsVehicle.SpriteViewToWindow( ptHotSpotTurret, *pspriteviewVehicle );

            drawparmsTurret.m_ptCenter = ptHotSpotClientTurret;
            drawparmsTurret.m_ptOffset -=
                ptAnchorTurret - CPoint( pspriteviewTurret->Width( ) / 2, pspriteviewTurret->Height( ) / 2 );

            rectTurret = pspriteviewTurret->Draw( drawparmsTurret );

            rectBound |= rectTurret;

            // Draw muzzle flash

            CUnitTile* ptileMuzzleFlash = GetTurret( )->GetMuzzleFlash( );

            if ( ptileMuzzleFlash && ptileMuzzleFlash->GetAmbient( CSpriteView::ANIM_FRONT_1 )->IsEnabled( ) )
            {
                CVehicleSprite* pspriteMuzzleFlash = (CVehicleSprite*)ptileMuzzleFlash->GetSprite( );

#ifdef _DEBUG
                pspriteMuzzleFlash->CheckValid( );
#endif

                CSpriteView* pspriteviewMuzzleFlash =
                    pspriteMuzzleFlash->GetView( iDirectionIndex, iTiltIndex, m_bDamage );

                CHotSpotKey     hotspotkeyMuzzleFlash( CHotSpotKey::MUZZLE_HOTSPOT, 0 );
                CHotSpot const* photspotMuzzleFlash = pspriteviewTurret->GetHotSpot( hotspotkeyMuzzleFlash );

                ASSERT( photspotMuzzleFlash );  // Turret had a flash sprite, so should also have hotspot

                CPoint ptAnchorMuzzleFlash  = pspriteviewMuzzleFlash->GetAnchorPoint( );
                CPoint ptHotSpotMuzzleFlash = pspriteviewTurret->GetHotSpotPoint( *photspotTurret );
                CPoint ptHotSpotClientMuzzleFlash =
                    drawparmsTurret.SpriteViewToWindow( ptHotSpotMuzzleFlash, *pspriteviewTurret );

                CQuadDrawParms drawparmsMuzzleFlash = drawparmsTurret;

                drawparmsMuzzleFlash.m_ptile    = ptileMuzzleFlash;
                drawparmsMuzzleFlash.m_ptCenter = ptHotSpotClientMuzzleFlash;
                drawparmsMuzzleFlash.m_ptOffset -=
                    ptAnchorMuzzleFlash -
                    CPoint( pspriteviewMuzzleFlash->Width( ) / 2, pspriteviewMuzzleFlash->Height( ) / 2 );

                rectBound |= pspriteviewMuzzleFlash->Draw( drawparmsMuzzleFlash );
            }
        }
    }

    // Draw flag

    if ( GetFlag( ) )
    {
        CPoint ptHotSpotClient;

        BOOL bHotSpot =
            GetHotSpotClient( *pspriteviewVehicle, drawparmsVehicle, CHotSpotKey::FLAG_HOTSPOT, 0, &ptHotSpotClient );

        if ( bHotSpot )
            rectBound |= GetFlag( )->Draw( ptHotSpotClient );
    }

    // Draw smoke/flame

    if ( GetDamageDisplay( ) )
        GetDamageDisplay( )->Draw( *pspriteviewVehicle, drawparmsVehicle );  // Self-invalidating

                                                                             // Draw selected corners

#ifdef _CHEAT
    BOOL bAISelected = GetOwner( )->IsAI( ) && 0 != _bShowAISelected;
#else
    const BOOL bAISelected = FALSE;
#endif

    if ( bAISelected || ( CUnit::selected & GetFlags( ) ) )
    {
#ifdef _CHEAT
        int iColor;

        if ( GetOwner( )->IsMe( ) )
            iColor = 255;
        else
            iColor = min( 254, 248 + GetOwner( )->GetPlyrNum( ) );
#else
        const int iColor = 255;
#endif

        CRect rectSelect = pspriteviewVehicle->Rect( );

        rectSelect.InflateRect( 1, 1 );

        CPoint ptOffset = drawparmsVehicle.m_ptCenter - rectSelect.TopLeft( ) -
                          CPoint( rectSelect.Width( ) >> 1, rectSelect.Height( ) >> 1 );

        rectSelect += ptOffset;

        if ( CDrawParms::IsDrawMode( ) )
            pspriteviewVehicle->DrawSelected( rectSelect, iColor );

        rectBound |= rectSelect;
    }

    if ( CDrawParms::IsInvalidateMode( ) )
        xpanimatr->GetDirtyRects( )->AddRect( &rectBound, CDirtyRects::RECT_LIST::LIST_PAINT_BOTH );

    return rectBound;
}

//---------------------------------------------------------------------------
// CVehicle::IsHit
//---------------------------------------------------------------------------
BOOL CVehicle::IsHit( CHexCoord, CPoint ptCursor ) const
{
    ASSERT_STRICT_VALID( this );

    CBridgeUnit* pbridge = GetBridgeOn( );

    int iTiltIndex      = GetTiltIndex( pbridge != NULL );
    int iDirectionIndex = GetDirectionIndex( m_iDir, CVehicleSprite::TILT_TYPE( iTiltIndex ) );

    CQuadDrawParms drawparmsVehicle;

    if ( !GetDrawParms( drawparmsVehicle, pbridge ) )
        return FALSE;

    CQuadDrawParms drawparmsTurret = drawparmsVehicle;

    AdjustDirection( drawparmsVehicle, iTiltIndex, iDirectionIndex, m_iDir );

    CSpriteView* pspriteviewTurret  = NULL;
    CSpriteView* pspriteviewVehicle = GetSprite( )->GetView( iDirectionIndex, iTiltIndex, m_bDamage );

    if ( CTransportData::troops == GetData( )->GetBaseType( ) )
        drawparmsVehicle.m_bTransparentHitTest = TRUE;

    if ( pspriteviewVehicle->IsHit( drawparmsVehicle, ptCursor ) )
        return TRUE;

    // Check turret

    if ( GetTurret( ) )
    {
        drawparmsTurret.m_ptile = ( (CVehicle*)this )->GetTurret( );

        iDirectionIndex = GetDirectionIndex( GetTurret( )->GetDir( ), CVehicleSprite::TILT_TYPE( iTiltIndex ) );

        AdjustDirection( drawparmsTurret, iTiltIndex, iDirectionIndex, GetTurret( )->GetDir( ) );

        pspriteviewTurret = GetTurret( )->GetSprite( )->GetView( iDirectionIndex, iTiltIndex, m_bDamage );

        CHotSpotKey     hotspotkey( CHotSpotKey::TURRET_HOTSPOT, 0 );
        CHotSpot const* photspot = pspriteviewVehicle->GetHotSpot( hotspotkey );

        if ( photspot )
        {
            // Place turret center over turret hotspot, then offset to align anchor with hotspot

            CPoint ptAnchor  = pspriteviewTurret->GetAnchorPoint( );
            CPoint ptHotSpot = pspriteviewVehicle->GetHotSpotPoint( *photspot );

            CPoint ptHotSpotClient = drawparmsVehicle.SpriteViewToWindow( ptHotSpot, *pspriteviewVehicle );

            drawparmsTurret.m_ptCenter = ptHotSpotClient;
            drawparmsTurret.m_ptOffset -=
                ptAnchor - CPoint( pspriteviewTurret->Width( ) / 2, pspriteviewTurret->Height( ) / 2 );
            if ( pspriteviewTurret->IsHit( drawparmsTurret, ptCursor ) )
                return TRUE;
        }
    }

    // Check smoke/flame

    if ( GetDamageDisplay( ) )
        return GetDamageDisplay( )->IsHit( *pspriteviewVehicle, drawparmsVehicle, ptCursor );

    return FALSE;
}

void CVehicle::InvalidateStatus( ) const
{

    ASSERT_VALID( this );

    if ( ( GetOwner( ) == NULL ) || ( !GetOwner( )->IsMe( ) ) )
        return;

    // invalidate any area windows
    theAreaList.InvalidateStatus( this );

    // invalidate the list box
    int iIndex = theApp.m_wndVehicles.FindItem( this );
    ASSERT( iIndex >= 0 );
    if ( iIndex < 0 )
        return;
    int iTop = theApp.m_wndVehicles.m_ListBox.GetTopIndex( );
    if ( iIndex < iTop )
        return;

    CRect rect( 0, 0, 0, 0 );
    theApp.m_wndVehicles.m_ListBox.GetClientRect( &rect );
    rect.top = ( iIndex - iTop ) * 64;
    if ( rect.top >= rect.bottom )
        return;
    rect.bottom = __min( rect.bottom, rect.top + 64 );
    theApp.m_wndVehicles.m_ListBox.InvalidateRect( &rect, FALSE );
}

void CVehicle::SetDestAndMode( CSubHex sub, VEH_POS iMode )
{
    const int aiAdd[4][2] = { 0, 1, 0, 0, 1, 0, 1, 1 };

    if ( m_pVehLoadOn != NULL )
        if ( theVehicleHex._GetVehicle( sub ) != m_pVehLoadOn )
            m_pVehLoadOn = NULL;

#ifdef _LOGOUT
    logPrintf( LOG_PRI_USEFUL, LOG_VEH_MOVE, "Vehicle %d at sub (%d,%d) SetDest to (%d,%d)", GetID( ), m_ptHead.x,
               m_ptHead.y, sub.x, sub.y );
#endif
    ASSERT_VALID( this );
    DeletePath( );
    m_hexLastDest = sub;

    // a goto for ANY reason means this goes away
    DestroyLoadWindow( );

    m_iTimesOn      = 0;
    m_iClosest      = INT_MAX;
    m_iNumClosest   = 0;
    m_iNumRetries   = 0;
    m_dwTimeBlocked = 0;
    m_iBlockCount   = 0;
    m_bFlags &= ~told_ai_stop;

    // for a building there is only one entrance
    CBuilding* pBldg = theBuildingHex._GetBuilding( sub );
    if ( ( pBldg != NULL ) && ( CanEnterBldg( pBldg ) ) )
    {
        if ( GetData( )->IsBoat( ) )
        {
            sub.x = pBldg->GetShipHex( ).X( ) * 2 + aiAdd[pBldg->GetShipDir( )][0];
            sub.y = pBldg->GetShipHex( ).Y( ) * 2 + aiAdd[pBldg->GetShipDir( )][1];
        }
        else
        {
            sub.x = pBldg->GetExitHex( ).X( ) * 2 + aiAdd[pBldg->GetExitDir( )][0];
            sub.y = pBldg->GetExitHex( ).Y( ) * 2 + aiAdd[pBldg->GetExitDir( )][1];
        }
        iMode = full;
    }

    m_hexDest   = sub.ToCoord( );
    m_ptDest    = sub;
    m_iDestMode = iMode;
    SetHexDest( );

    // if not there in 6 minutes we jump it there
    m_dwTimeJump = theGame.GettimeGetTime( ) + 1000 * TRUCK_JUMP_TIME;

    // may be there
    if ( m_ptDest == m_ptHead )
    {
        _SetRouteMode( moving );
        return;
    }

    if ( !m_cOwn )
    {
        ASSERT( theVehicleHex.GetVehicle( m_ptHead ) != this );
        ASSERT( theVehicleHex.GetVehicle( m_ptTail ) != this );

        // if we are in a building
        if ( theBuildingHex._GetBuilding( m_ptHead ) != NULL )
        {
            ExitBuilding( );

            // if not deployed we have to wait
            if ( ( m_cMode == cant_deploy ) || ( !m_cOwn ) )
            {
                if ( m_cMode != cant_deploy )
                    _SetRouteMode( cant_deploy );
                return;
            }
        }

        else
            // this is unload or place
            if ( ( theVehicleHex._GetVehicle( m_ptHead ) == NULL ) &&
                 ( theVehicleHex._GetVehicle( m_ptTail ) == NULL ) )
            {
                TakeOwnership( );
                SetLoc( TRUE );
                GetPath( FALSE );
            }
            else
            // have to wait for a space
            {
                if ( m_cMode != cant_deploy )
                    _SetRouteMode( cant_deploy );
                return;
            }
    }

#ifdef _DEBUG
    else
    {
        ASSERT( theVehicleHex.GetVehicle( m_ptHead ) == this );
        ASSERT( theVehicleHex.GetVehicle( m_ptTail ) == this );
    }
#endif

    // if we're moving just change the path
    ASSERT( ( m_cMode != moving ) || ( m_iStepsLeft > 0 ) );
    if ( ( m_cMode == moving ) && ( m_iStepsLeft > 0 ) )
    {
        ASSERT( m_ptNext != m_ptHead );
        ASSERT( ( abs( CSubHex::Diff( m_ptNext.x - m_ptHead.x ) ) < 2 ) &&
                ( abs( CSubHex::Diff( m_ptNext.y - m_ptHead.y ) ) < 2 ) );
        if ( !HavePathOrNext( ) )
            GetPath( FALSE );
    }
    else

    {
        _SetRouteMode( moving );
        KickStart( );
    }
}

void CVehicle::KickStart( )
{

    ASSERT_VALID( this );

    // if not moving this can't do anything
    if ( m_cMode != moving )
        return;

    StartTravel( TRUE );
}

void CVehicle::DeletePath( )
{

    delete[] m_phexPath;
    m_phexPath = NULL;
    m_iPathOff = m_iPathLen = 0;

    // stops bogus returns on HavePathOrNext
    m_hexNext = m_ptHead;
}

BOOL CVehicle::HavePathOrNext( ) const
{

    if ( m_iPathOff < m_iPathLen )
        return ( TRUE );

    if ( ( m_iPathOff >= m_iPathLen ) || ( m_phexPath == NULL ) )
    {
        CHexCoord _hexHead( m_ptHead );
        if ( ( abs( CHexCoord::Diff( m_hexNext.X( ) - _hexHead.X( ) ) ) <= 1 ) &&
             ( abs( CHexCoord::Diff( m_hexNext.Y( ) - _hexHead.Y( ) ) ) <= 1 ) )
            if ( ( !m_hexNext.SameHex( m_ptHead ) ) ||
                 ( ( abs( CHexCoord::Diff( m_hexNext.X( ) - m_hexDest.X( ) ) ) <= 1 ) &&
                   ( CHexCoord::Diff( abs( m_hexNext.Y( ) - m_hexDest.Y( ) ) <= 1 ) ) ) )
                return ( TRUE );
    }

    return ( FALSE );
}

BOOL CVehicle::HavePath( ) const
{

    if ( m_iPathOff < m_iPathLen )
        return ( TRUE );
    return ( FALSE );
}

void CVehicle::GetPath( BOOL bNoOcc )
{

#ifdef _DEBUG
    ASSERT_VALID( this );
    ASSERT( GetOwner( )->IsLocal( ) );
#endif

    CHexCoord _dest;
    if ( ( m_iPathOff >= m_iPathLen ) && ( m_iPathLen > 0 ) )
        _dest = *( m_phexPath + m_iPathLen - 1 );
    else
        _dest = m_hexDest;

    // get the path
    delete[] m_phexPath;
    m_iPathOff = 0;

    // for a building we use the exit hex as the src/dest
    CHexCoord  _hexSrc = GetHexHead( );
    CBuilding* pBldg   = theBuildingHex._GetBuilding( _hexSrc );
    if ( ( pBldg != NULL ) && ( pBldg->GetOwner( ) == GetOwner( ) ) )
        _hexSrc = pBldg->GetExit( GetData( )->GetWheelType( ) );

    CHexCoord _hexDest = GetHexDest( );
    pBldg              = theBuildingHex._GetBuilding( _hexDest );
    if ( ( pBldg != NULL ) && ( pBldg->GetOwner( ) == GetOwner( ) ) )
        _hexDest = pBldg->GetExit( GetData( )->GetWheelType( ) );

    // get the path (fake one up if same or adjoining)
    int xDif = CHexCoord::Diff( _hexDest.X( ) - _hexSrc.X( ) );
    int yDif = CHexCoord::Diff( _hexDest.Y( ) - _hexSrc.Y( ) );
    if ( ( abs( xDif ) <= 1 ) && ( abs( yDif ) <= 1 ) )
    {
        m_phexPath  = new CHexCoord[3];
        *m_phexPath = _hexSrc;
        m_iPathLen  = 1;
        CHexCoord _tmp( _hexSrc.X( ) + xDif, _hexSrc.Y( ) + yDif );
        _tmp.Wrap( );
        if ( *m_phexPath != _tmp )
        {
            *( m_phexPath + 1 ) = _tmp;
            m_iPathLen          = 2;
        }
        if ( *( m_phexPath + m_iPathLen - 1 ) != _hexDest )
        {
            *( m_phexPath + 1 ) = _hexDest;
            m_iPathLen++;
        }
    }
    else
        m_phexPath = thePathMgr.GetPath( this, _hexSrc, _hexDest, m_iPathLen, 0, bNoOcc );

    // if we have no path we're stuck
    if ( ( m_iPathLen <= 0 ) || ( ( m_iPathLen > 1 ) && ( *m_phexPath == *( m_phexPath + m_iPathLen - 1 ) ) ) )
    {
        _SetRouteMode( blocked );
        m_iNumRetries = MAX_NUM_RETRIES;
        m_iBlockCount = 6;
        return;
    }

    // get hexNext
    if ( m_iPathLen > 0 )
    {
        m_hexNext = *m_phexPath;
        PathNextHex( );
    }

    // is it the same as last time?
    CHexCoord _newDest( *( m_phexPath + m_iPathLen - 1 ) );
    if ( ( _newDest != _hexDest ) && ( _newDest == m_hexLastDest ) )
    {
        delete[] m_phexPath;
        m_phexPath = NULL;
        m_iPathOff = m_iPathLen = 0;
        _SetRouteMode( blocked );
        m_iNumRetries = MAX_NUM_RETRIES;
        m_iBlockCount = 6;
#ifdef _LOGOUT
        logPrintf( LOG_PRI_USEFUL, LOG_VEH_MOVE, "Vehicle %d can't reach dest, same path as 2 ago", GetID( ) );
#endif
    }
    m_hexLastDest = _dest;
}

void CVehicle::SetLocation( CHexCoord& hex, POSITION pos, int iType )
{

    ASSERT_VALID( this );

    // if its a building get it's entrance
    hex.Wrap( );
    CBuilding* pBldg = theBuildingHex._GetBuilding( hex );
    if ( pBldg != NULL )
    {
        if ( GetData( )->IsBoat( ) )
            hex = pBldg->GetShipHex( );
        else
            hex = pBldg->GetExitHex( );
    }

    CRoute* pR = new CRoute( hex, iType );
    if ( pos == NULL )
        m_route.AddHead( pR );
    else

    {
        POSITION pos_on = m_route.GetHeadPosition( );
        while ( ( pos_on != NULL ) && ( pos_on != pos ) ) m_route.GetNext( pos_on );
        if ( pos_on == NULL )
        {
            TRAP( );  // should this be possible?
            m_route.AddTail( pR );
        }
        else
            m_route.InsertAfter( pos_on, pR );
    }

    if ( GetRoutePos( ) == NULL )
        SetRoutePos( m_route.GetHeadPosition( ) );
}

void CVehicle::SetEvent( VEH_EVENT iEvent )
{

    ASSERT_VALID( this );

    m_iEvent = iEvent;
    switch ( iEvent )
    {
    case attack:
        m_ptUs = m_ptHead;
        m_ptUs.x += 8;
        m_ptUs.Wrap( );
        break;

    case route:
        if ( m_pos == NULL )
            m_pos = m_route.GetHeadPosition( );
        if ( m_pos != NULL )
        {
            CRoute* pR = m_route.GetAt( m_pos );
            if ( pR == NULL )
            {
                TRAP( );
                if ( ( m_pos = m_route.GetHeadPosition( ) ) != NULL )
                {
                    pR = m_route.GetAt( m_pos );
                    if ( pR == NULL )
                    {
                        TRAP( );  // should never happen
                        return;
                    }
                }
            }
            ASSERT_VALID( pR );

            if ( m_hexDest != pR->GetCoord( ) )
                SetDestAndMode( pR->GetCoord( ), full );
        }
        break;

    case build_road:
        m_iBuildDone = 0;
        m_lOperMod   = 0;
        m_iLastPer   = 0;
        break;
    }
}

void CVehicle::SetRouteMode( VEH_MODE iMode )
{

    ASSERT_VALID( this );

    // if can't deploy we have to wait
    if ( m_cMode == cant_deploy )
        return;

    // if stopped - set it to where we are
    if ( ( iMode == stop ) && ( m_cMode == moving ) )
    {
        iMode = moving;
        if ( m_iEvent != attack )
            m_iEvent = none;
        SetDestAndMode( m_ptNext, sub );
    }
    else if ( ( iMode == moving ) && ( m_cMode != moving ) )
    {
        // if m_hexNext is wrong, delete the path
        if ( HavePath( ) )
        {
            TRAP( );
            if ( *( m_phexPath + m_iPathOff - 1 ) != m_hexDest )
            {
                TRAP( );
                DeletePath( );
            }
        }

        TRAP( );
        StartTravel( TRUE );
    }

    _SetRouteMode( iMode );

    // update area list buttons
    theAreaList.MaterialChange( this );
}

// make sure we are positioned right
void CVehicle::CheckExit( )
{

    // if the exit is passable we're done
    if ( IsPassable( m_ptNext, FALSE ) )
        return;

    // if we're not in a building we're done
    CBuilding* pBldg = theBuildingHex._GetBuilding( m_ptHead );
    if ( pBldg == NULL )
        return;

    for ( int x = -1; x < pBldg->GetCX( ) + 1; x++ )
        for ( int y = -1; y < pBldg->GetCX( ) + 1; y++ )
        {
            CHexCoord _next( pBldg->GetHex( ).X( ) + x, pBldg->GetHex( ).Y( ) + y );
            _next.Wrap( );

            if ( ( theBuildingHex._GetBuilding( _next ) == NULL ) &&
                 ( GetData( )->CanEnterHex( m_ptHead, _next, IsOnWater( ), FALSE ) ) )
            {
                if ( x < 0 )
                {
                    m_ptHead.x = pBldg->GetHex( ).X( ) * 2;
                    m_ptTail.x = m_ptHead.x + 1;
                    m_ptHead.y = m_ptTail.y = _next.Y( ) * 2;
                }
                else if ( y < 0 )
                {
                    m_ptHead.y = pBldg->GetHex( ).Y( ) * 2;
                    m_ptTail.y = m_ptHead.y + 1;
                    m_ptHead.x = m_ptTail.x = _next.X( ) * 2;
                }
                else if ( x >= pBldg->GetCX( ) )
                {
                    TRAP( );
                    m_ptHead.x = ( pBldg->GetHex( ).X( ) + pBldg->GetCX( ) - 1 ) * 2 + 1;
                    m_ptTail.x = m_ptHead.x - 1;
                    m_ptHead.y = m_ptTail.y = _next.Y( ) * 2;
                }
                else
                {
                    m_ptHead.y = ( pBldg->GetHex( ).Y( ) + pBldg->GetCY( ) - 1 ) * 2 + 1;
                    m_ptTail.y = m_ptHead.y - 1;
                    m_ptHead.x = m_ptTail.x = _next.X( ) * 2;
                }

                // if free is a diag then we can be off
                m_ptHead.x = __minmax( pBldg->GetHex( ).X( ) * 2,
                                       ( pBldg->GetHex( ).X( ) + pBldg->GetCX( ) - 1 ) * 2 + 1, m_ptHead.x );
                m_ptHead.y = __minmax( pBldg->GetHex( ).Y( ) * 2,
                                       ( pBldg->GetHex( ).Y( ) + pBldg->GetCY( ) - 1 ) * 2 + 1, m_ptHead.y );
                m_ptHead.Wrap( );
                m_ptTail.x = __minmax( pBldg->GetHex( ).X( ) * 2,
                                       ( pBldg->GetHex( ).X( ) + pBldg->GetCX( ) - 1 ) * 2 + 1, m_ptTail.x );
                m_ptTail.y = __minmax( pBldg->GetHex( ).Y( ) * 2,
                                       ( pBldg->GetHex( ).Y( ) + pBldg->GetCY( ) - 1 ) * 2 + 1, m_ptTail.y );
                m_ptTail.Wrap( );
                m_hexNext = m_ptNext = Rotate( 0 );
                if ( GetData( )->GetVehFlags( ) & CTransportData::FL1hex )
                    m_ptTail = m_ptHead;
                if ( IsPassable( m_ptNext, FALSE ) )
                    goto FoundIt;
            }
        }

FoundIt:
    SetMoveParams( FALSE );
}

void CVehicle::_SetRouteMode( VEH_MODE iMode )
{

    ASSERT_VALID( this );

    BOOL bOld = ( m_cMode == moving );
    m_cMode   = iMode;

    BOOL bNew = ( iMode == moving );
    if ( bOld != bNew )
        Wheels( bNew, TRUE );

    // turn off stop
    if ( ( iMode != stop ) && ( m_cMode == stop ) )
    {
        TRAP( );  // BUGBUG - this should be above I think
        ResumeUnit( );
    }

    // invalidate route window
    if ( m_pWndRoute != NULL )
        m_pWndRoute->Invalidate( );

    switch ( m_cMode )
    {
    // nothing happening
    case stop:
        m_lOperMod = 0;
        m_bFlags &= ~told_ai_stop;
        break;

    case cant_deploy: {
        // make sure we are positioned right
        CBuilding* pBldg = theBuildingHex._GetBuilding( m_ptHead );
        if ( pBldg != NULL )
        {
            GetExitLoc( pBldg, GetData( )->GetType( ), m_ptNext, m_ptHead, m_ptTail );
            // if the exit isn't passable find another
            CheckExit( );
            m_hexNext = m_ptNext;
        }
    }  // no break

    case deploy_it:
        ASSERT_VALID_LOC( this );
#ifndef _GG
        ASSERT( theBuildingHex.GetBuilding( m_ptHead ) != NULL );
#endif

    case traffic:
#ifdef TEST_TRAFFIC
        if ( m_cOwn )
        {
            TRAP( theVehicleHex._GetVehicle( m_ptHead ) != this );
            TRAP( theVehicleHex._GetVehicle( m_ptTail ) != this );
            for ( int iInd = 0; iInd < NUM_SUBS_OWNED; iInd++ )
                if ( ( SubsOwned[iInd].x != -1 ) && ( SubsOwned[iInd] != m_ptNext ) &&
                     ( SubsOwned[iInd] != m_ptHead ) && ( SubsOwned[iInd] != m_ptTail ) )
                    TRAP( );
        }
#endif
    case blocked:
    case contention:
        m_lOperMod      = 0;
        m_dwTimeBlocked = 0;
        break;

    case moving:
        m_lOperMod = 0;
        break;
    }

    // we may have interrupted part way through
    SetLoc( TRUE );

    ASSERT( ( m_cMode != moving ) || ( m_cOwn ) );
}

void CVehicle::SetRoutePos( POSITION pos )
{

    ASSERT_VALID( this );

    m_pos = pos;

    if ( m_pWndRoute != NULL )
        m_pWndRoute->Invalidate( );
}

void CVehicle::PathNextHex( )
{

    // if we are next to the dest - use the dest
    if ( ( m_iPathOff >= m_iPathLen ) || ( m_phexPath == NULL ) )
    {
        if ( ( abs( m_hexNext.X( ) - m_hexDest.X( ) ) <= 1 ) && ( abs( m_hexNext.Y( ) - m_hexDest.Y( ) ) <= 1 ) )
        {
            m_hexNext = m_hexDest;
            return;
        }

        // if we have no path we're going nowhere
        m_hexNext = GetHexHead( );
        return;
    }

    ASSERT( m_iPathOff < m_iPathLen );

    while ( ( m_hexNext.SameHex( m_ptHead ) ) && ( m_iPathOff < m_iPathLen ) )
    {
        m_hexNext = *( m_phexPath + m_iPathOff );
        m_iPathOff++;
    }
}

void CVehicle::SetBuilding( CHexCoord const& hex, int iBldgType, int iDir )
{

    ASSERT_VALID( this );

    m_iEvent    = build;
    m_hexBldg   = hex;
    m_iBldgType = iBldgType;
    m_iBuildDir = iDir;

    ASSERT_VALID( this );
}

void CVehicle::SetRoad( const CHexCoord& hexSrc, const CHexCoord& hexDest )
{

    ASSERT_VALID( this );

    m_hexStart = hexSrc;
    m_hexEnd   = hexDest;

    SetEvent( build_road );
    SetDestAndMode( hexSrc, center );
}

void CVehicle::TakeOwnership( )
{

    if ( !m_cOwn )
    {
        ASSERT_VALID( this );
        m_cOwn = TRUE;
        ASSERT( theVehicleHex.GetVehicle( m_ptHead ) == NULL );
        ASSERT( theVehicleHex.GetVehicle( m_ptTail ) == NULL );
        ASSERT( theVehicleHex.GetVehicle( m_ptNext ) == NULL );

        theVehicleHex.GrabHex( m_ptNext, this );
        if ( m_ptHead != m_ptNext )
            theVehicleHex.GrabHex( m_ptHead, this );
        if ( m_ptTail != m_ptHead )
            theVehicleHex.GrabHex( m_ptTail, this );

        // only me
        if ( ( m_pOwner != NULL ) || ( m_pOwner->IsMe( ) ) )
            if ( DoSpotting( ) )
            {
                DetermineSpotting( );
                IncrementSpotting( GetHexHead( ) );
            }

        // update oppo fire
        if ( GetOwner( )->IsLocal( ) )
            OppoAndOthers( );
    }

    ASSERT_VALID( this );
}

void CVehicle::ReleaseOwnership( )
{

    ASSERT_VALID( this );
    ASSERT( GetOwner( )->IsLocal( ) );

    if ( m_cOwn )
    {
        ASSERT( theVehicleHex.GetVehicle( m_ptHead ) == this );
        ASSERT( theVehicleHex.GetVehicle( m_ptTail ) == this );

        theVehicleHex.ReleaseHex( m_ptHead, this );
        if ( m_ptHead != m_ptTail )
            theVehicleHex.ReleaseHex( m_ptTail, this );
        if ( theVehicleHex._GetVehicle( m_ptNext ) == this )
            theVehicleHex.ReleaseHex( m_ptNext, this );

        m_cOwn = FALSE;

        // undo all spotting
        if ( DoSpotting( ) && SpottingOn( ) )
            DecrementSpotting( );

        // they can't shoot at me - I'm inside now
        CUnit* pUnit;
        if ( m_pTransport != NULL )
            pUnit = m_pTransport;
        else
            pUnit = theBuildingHex._GetBuilding( m_ptHead );
        StopShooting( pUnit );
    }

    ASSERT_VALID( this );
}

void CVehicle::StopUnit( )
{

    CUnit::StopUnit( );

    SetEvent( CVehicle::none );
    SetDest( GetPtNext( ) );
}

void CVehicle::ResumeUnit( )
{

    CUnit::ResumeUnit( );

    if ( GetRouteList( ).GetCount( ) > 0 )
        SetEvent( CVehicle::route );
}

//---------------------------------------------------------------------------
// CTree::GetDrawCorner
//---------------------------------------------------------------------------
CPoint CTree::GetDrawCorner( const CHexCoord& hexcoord ) const
{
    CPoint aptHex[4];

    xpanimatr->MapToWindowHex( hexcoord, aptHex );

    int iX = aptHex[0].x;
    int iY = aptHex[0].y + ( MAX_HEX_HT >> 1 + xiZoom );

    return CPoint( iX, iY );
}

//---------------------------------------------------------------------------
// CTree::Draw
//---------------------------------------------------------------------------
CRect CTree::Draw( const CHexCoord& hexcoord )
{
    CHex*   phex = theMap.GetHex( hexcoord );
    CMapLoc maploc( hexcoord );

    ASSERT_VALID( phex );

    CSpriteView* pspriteview = GetSprite( )->GetView( GetLayer( ) );

    CPoint ptOffset = GetDrawCorner( hexcoord );

    ptOffset.y -= pspriteview->Height( );

    CDrawParms drawparms( *this, ptOffset );

    return pspriteview->Draw( drawparms );
}

/////////////////////////////////////////////////////////////////////////////
// CUnitHex

static int fnEnumGrab( CHex*, CHexCoord hex, void* pData )
{

    theBuildingHex._GrabHex( hex.X( ), hex.Y( ), (CBuilding*)pData );
    return ( FALSE );
}

void CBuildingHex::GrabHex( CBuilding const* pBldg )
{

    theMap.EnumHexes( pBldg->GetHex( ), pBldg->GetCX( ), pBldg->GetCY( ), fnEnumGrab, (void*)pBldg );
}

static int fnEnumRelease( CHex*, CHexCoord hex, void* )
{

    theBuildingHex._ReleaseHex( hex.X( ), hex.Y( ) );
    return ( FALSE );
}

void CBuildingHex::ReleaseHex( CBuilding const* pBldg )
{

    theMap.EnumHexes( pBldg->GetHex( ), pBldg->GetCX( ), pBldg->GetCY( ), fnEnumRelease, NULL );
}


/////////////////////////////////////////////////////////////////////////////
//

CUnit* _GetUnit( DWORD dwID )
{

    CUnit* pRtn = theBuildingMap._GetBldg( dwID );
    if ( pRtn != NULL )
        return ( pRtn );

    return ( theVehicleMap._GetVehicle( dwID ) );
}

CUnit* GetUnit( DWORD dwID )
{

    CUnit* pRtn = theBuildingMap.GetBldg( dwID );
    if ( pRtn != NULL )
    {
        if ( !( pRtn->GetFlags( ) & CUnit::dying ) )
            return ( pRtn );
        TRAP( );
    }

    pRtn = theVehicleMap.GetVehicle( dwID );
    if ( pRtn == NULL )
        return ( NULL );

    if ( !( pRtn->GetFlags( ) & CUnit::dying ) )
        return ( pRtn );
    return ( NULL );
}

CUnit* GetUnit( CSubHex const& sub )  // bldg takes priority
{

    CBuilding* pBldg = theBuildingHex.GetBuilding( sub );
    if ( pBldg != NULL )
    {
        if ( !( pBldg->GetFlags( ) & CUnit::dying ) )
            return ( pBldg );
        TRAP( );
    }

    CVehicle* pVeh = theVehicleHex.GetVehicle( sub );
    if ( pVeh == NULL )
        return ( NULL );

    if ( !( pVeh->GetFlags( ) & CUnit::dying ) )
        return ( pVeh );
    return ( NULL );
}

// rotate 0 - 7 (actually -4 - 3)
CSubHex Rotate( int iDir, CSubHex const& ptHead, CSubHex const& ptTail )
{

#ifdef _DEBUG
    int d = iDir;
#endif

    iDir = ( ( iDir + 4 ) & 0x07 ) - 4;

    if ( ptHead == ptTail )
    {
        CSubHex _rtn( ptHead );

        switch ( ( iDir + 8 ) & 7 )
        {
        case 0:
            _rtn.y--;
            break;
        case 1:
            _rtn.x++;
            _rtn.y--;
            break;
        case 2:
            _rtn.x++;
            break;
        case 3:
            _rtn.x++;
            _rtn.y++;
            break;
        case 4:
            _rtn.y++;
            break;
        case 5:
            _rtn.x--;
            _rtn.y++;
            break;
        case 6:
            _rtn.x--;
            break;
        case 7:
            _rtn.x--;
            _rtn.y--;
            break;
        default:
            ASSERT( FALSE );
            _rtn.y--;
            break;
        }

        _rtn.Wrap( );
        return ( _rtn );
    }

    int dX = CSubHex::Diff( ptHead.x - ptTail.x );
    int dY = CSubHex::Diff( ptHead.y - ptTail.y );
    int xStep, yStep;

    switch ( iDir )
    {
    case 4:
    case -4:
        xStep = -2 * dX;
        yStep = -2 * dY;
        break;
    case -3:
        TRAP( dX == 0 && dY == 0 );
        xStep = -dX + dY;
        xStep = ( xStep < -1 ) ? -1 : ( xStep > 1 ? 1 : xStep );
        yStep = -dX - dY;
        yStep = ( yStep < -1 ) ? -1 : ( yStep > 1 ? 1 : yStep );
        break;
    case -2:
        xStep = dY;
        yStep = -dX;
        break;
    case -1:
        xStep = dX + dY;
        xStep = ( xStep < -1 ) ? -1 : ( xStep > 1 ? 1 : xStep );
        yStep = -dX + dY;
        yStep = ( yStep < -1 ) ? -1 : ( yStep > 1 ? 1 : yStep );
        break;
    case 0:
        xStep = dX;
        yStep = dY;
        break;
    case 1:
        xStep = dX - dY;
        xStep = ( xStep < -1 ) ? -1 : ( xStep > 1 ? 1 : xStep );
        yStep = dX + dY;
        yStep = ( yStep < -1 ) ? -1 : ( yStep > 1 ? 1 : yStep );
        break;
    case 2:
        xStep = -dY;
        yStep = dX;
        break;
    case 3:
        TRAP( dX == 0 && dY == 0 );
        xStep = -dX - dY;
        xStep = ( xStep < -1 ) ? -1 : ( xStep > 1 ? 1 : xStep );
        yStep = dX - dY;
        yStep = ( yStep < -1 ) ? -1 : ( yStep > 1 ? 1 : yStep );
        break;

    default:
        ASSERT( FALSE );
        xStep = yStep = 0;
        break;
    }

    CSubHex _rtn( ptHead.x + xStep, ptHead.y + yStep );
    _rtn.Wrap( );

#ifdef _DEBUG
    if ( abs( iDir ) != 4 )
        ASSERT( iDir == GetAngle( _rtn, ptHead, ptHead, ptTail ) );
#endif

    return ( _rtn );
}

BOOL CMaterialBuilding::IsOperating( ) const
{

    if ( !CBuilding::IsOperating( ) )
        return FALSE;

    CBuildMaterials const* pBm = GetData( )->GetBldMaterials( );

    // materials to build it?
    for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ )
        if ( GetStore( iInd ) < pBm->GetInput( iInd ) )
            return FALSE;

    return TRUE;
}

BOOL CVehicleBuilding::IsOperating( ) const
{

    if ( !CBuilding::IsOperating( ) )
        return FALSE;

    if ( m_pBldUnt == NULL )
        return FALSE;

    return TRUE;
}

BOOL CRepairBuilding::IsOperating( ) const
{

    if ( !CBuilding::IsOperating( ) )
        return FALSE;

    if ( m_pVehRepairing == NULL )
        return FALSE;

    return TRUE;
}

BOOL CShipyardBuilding::IsOperating( ) const
{

    if ( !CBuilding::IsOperating( ) )
        return FALSE;

    if ( m_pVehRepairing != NULL )
        return TRUE;

    return CVehicleBuilding::IsOperating( );
}

BOOL CPowerBuilding::IsOperating( ) const
{

    if ( !CBuilding::IsOperating( ) )
        return FALSE;

    CBuildPower* pBp = GetData( )->GetBldPower( );

    if ( pBp->GetInput( ) < 0 )
        return TRUE;

    CBuildMaterials const* pBm = GetData( )->GetBldMaterials( );

    // materials to run it?
    if ( GetStore( pBp->GetInput( ) ) <= 0 )
        return FALSE;

    return TRUE;
}
