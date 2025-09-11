//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// netapi.cpp : implementation file
//

#include "netcmd.h"

#include "building.inl"
#include "lastplnt.h"
#include "player.h"
#include "stdafx.h"
#include "terrain.inl"
#include "unit.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


/////////////////////////////////////////////////////////////////////////////
// set up messages

CNetReady::CNetReady( CInitData* pId ): CNetCmd( cmd_ready )
{

    m_iPlyrNum = theGame.GetMe( )->GetNetNum( );
    m_InitData = *pId;

    ASSERT_CMD( this );
}

CNetYouAre::CNetYouAre( int iPlyr ): CNetCmd( cmd_you_are )
{

    m_iPlyrNum = iPlyr;
    if ( theGame.GetServer( ) == NULL )
        m_iServerNum = 0;
    else
        m_iServerNum = theGame.GetServer( )->GetPlyrNum( );
    ASSERT_CMD( this );
}

CNetPlyrJoin* CNetPlyrJoin::Alloc( const CNetPlyrJoin* pData )
{

    CNetPlyrJoin* pRtn = (CNetPlyrJoin*)new char[pData->m_iLen];

    pRtn->m_bMsg = CNetCmd::cmd_plyr_join;

    pRtn->m_iLen     = pData->m_iLen;
    pRtn->m_iPlyrNum = pData->m_iPlyrNum;
    pRtn->m_iPwrHave = pData->m_iPwrHave;
    pRtn->m_iPwrNeed = pData->m_iPwrNeed;
    pRtn->m_iPplHave = pData->m_iPplHave;
    pRtn->m_iPplNeed = pData->m_iPplNeed;
    for ( int iOn = 0; iOn < CMaterialTypes::num_types; iOn++ ) pRtn->m_iMat[iOn] = pData->m_iMat[iOn];
    pRtn->m_iRsrchLevel = pData->m_iRsrchLevel;
    pRtn->m_iNumBldgs   = pData->m_iNumBldgs;
    pRtn->m_iNumVeh     = pData->m_iNumVeh;
    pRtn->m_bAvail      = pData->m_bAvail;
    strcpy( pRtn->m_sName, pData->m_sName );

    return ( pRtn );
}

CNetPlyrJoin* CNetPlyrJoin::Alloc( const CPlayer* pPlyr )
{

    int           iLen = sizeof( CNetPlyrJoin ) + strlen( pPlyr->GetName( ) ) + 2;
    CNetPlyrJoin* pRtn = (CNetPlyrJoin*)new char[iLen];

    pRtn->m_bMsg = CNetCmd::cmd_plyr_join;

    pRtn->m_iLen     = iLen;
    pRtn->m_iPlyrNum = pPlyr->GetPlyrNum( );
    pRtn->m_iPwrHave = pPlyr->GetPwrHave( );
    pRtn->m_iPwrNeed = pPlyr->GetPwrNeed( );
    pRtn->m_iPplHave = pPlyr->GetPplBldg( );
    pRtn->m_iPplNeed = pPlyr->GetPplNeedBldg( );

    for ( int iOn = 0; iOn < CMaterialTypes::num_types; iOn++ ) pRtn->m_iMat[iOn] = pPlyr->GetMaterialHave( iOn );
    pRtn->m_iMat[CMaterialTypes::food] = pPlyr->GetFood( );
    pRtn->m_iMat[CMaterialTypes::gas]  = pPlyr->GetGasHave( );

    pRtn->m_iRsrchLevel = 0;
    for ( int iOn = 0; iOn < CRsrchArray::num_types; iOn++ )
        if ( ( ( (CPlayer*)pPlyr )->GetRsrch( iOn ).m_bDiscovered ) && ( theRsrch[iOn].m_iPtsRequired > 0 ) )
            pRtn->m_iRsrchLevel++;

    pRtn->m_iNumBldgs = pPlyr->GetBldgsHave( );
    pRtn->m_iNumVeh   = pPlyr->GetVehsHave( );
    strcpy( pRtn->m_sName, pPlyr->GetName( ) );

    if ( theApp.m_pCreateGame != NULL )
        pRtn->m_bAvail = pPlyr->GetState( ) != CPlayer::ready;
    else
        pRtn->m_bAvail = pPlyr->IsAI( );

    return ( pRtn );
}

CNetPlayer* CNetCmd::AllocPlayer( CPlayer* pPlr )
{

    ASSERT_VALID( pPlr );

    int         iLen = sizeof( CNetPlayer ) + strlen( pPlr->GetName( ) ) + 1;
    CNetPlayer* pMsg = (CNetPlayer*)new char[iLen];
    pMsg->m_iLen     = iLen;
    pMsg->m_bMsg     = CNetCmd::cmd_player;
    pMsg->m_iPlyrNum = pPlr->GetPlyrNum( );
    pMsg->m_iNetNum  = pPlr->GetNetNum( );
    pMsg->m_InitData = pPlr->m_InitData;
    pMsg->m_bAI      = pPlr->IsAI( );
    pMsg->m_bLocal   = FALSE;
    pMsg->m_bServer  = FALSE;
    strcpy( pMsg->m_sName, pPlr->GetName( ) );

    ASSERT_CMD( pMsg );
    return ( pMsg );
}

CNetChat* CNetChat::Alloc( const CPlayer* pPlyr, const char* psMsg )
{

    int       iLen = strlen( psMsg ) + 1;
    CNetChat* pRtn = (CNetChat*)new char[sizeof( CNetChat ) + iLen + 1];
    pRtn->m_iLen   = sizeof( CNetChat ) + iLen;

    pRtn->m_bMsg = CNetCmd::cmd_chat;
    if ( pPlyr != NULL )
        pRtn->m_iPlyrNetNum = pPlyr->GetNetNum( );
    else
        pRtn->m_iPlyrNetNum = theGame.GetMyNetNum( );
    memcpy( pRtn->m_sMsg, psMsg, iLen );

    return ( pRtn );
}

CNetStart::CNetStart( unsigned uRand, int iSide, int iSideSize, int iAiDiff, int iNumAi, int iNumHp, int iStart )
    : CNetCmd( cmd_start )
{

    m_uRand     = uRand;
    m_iSide     = iSide;
    m_iSideSize = iSideSize;
    m_iAi       = iAiDiff;
    m_iNumAi    = iNumAi;
    m_iNumHp    = iNumHp;
    m_iStart    = iStart;
    ASSERT_CMD( this );
}

CNetInitDone::CNetInitDone( CPlayer const* pPlyr ): CNetCmd( cmd_init_done )
{

    m_iPlyrNum = pPlyr->GetNetNum( );
}

CNetToAi::CNetToAi( CPlayer* pPlyr ): CNetCmd( cmd_to_ai )
{

    m_iPlyrNum = pPlyr->GetNetNum( );
}

CNetGetFile::CNetGetFile( CPlayer* pPlyr, CPlayer* pPlyrSrvr, int iBufLen ): CNetCmd( cmd_get_file )
{

    m_iPlyrNum      = pPlyr->GetPlyrNum( );
    m_iServerNum    = pPlyrSrvr->GetPlyrNum( );
    m_iServerNetNum = pPlyrSrvr->GetNetNum( );
    m_iBufLen       = iBufLen;
}


/////////////////////////////////////////////////////////////////////////////
// game messages

void _CMsgBldg::ctor( )
{

    m_iPlyrNum = 0;
    m_dwIDBldg = 0;
    m_dwIDVeh  = 0;
    m_iDir     = 0;
    m_iType    = 0;
    m_iWhy     = 0;
    m_bShow    = FALSE;
}

void _CMsgVeh::ctor( )
{

    m_dwIDBldg   = 0;
    m_dwID       = 0;
    m_iPlyrNum   = 0;
    m_iType      = 0;
    m_iRouteMode = 0;
    m_iDelay     = 0;
}

const _CMsgVehGo _CMsgVehGo::operator=( _CMsgVehGo const& src )
{

    m_dwID       = src.m_dwID;
    m_iPlyrNum   = src.m_iPlyrNum;
    m_ptDest     = src.m_ptDest;
    m_ptNext     = src.m_ptNext;
    m_ptHead     = src.m_ptHead;
    m_ptTail     = src.m_ptTail;
    m_hexNext    = src.m_hexNext;
    m_hexDest    = src.m_hexDest;
    m_iDir       = src.m_iDir;
    m_iTurretDir = src.m_iTurretDir;
    m_iXadd      = src.m_iXadd;
    m_iYadd      = src.m_iYadd;
    m_iDadd      = src.m_iDadd;
    m_iTadd      = src.m_iTadd;
    m_iMode      = src.m_iMode;
    m_iOwn       = src.m_iOwn;
    m_iStepsLeft = src.m_iStepsLeft;
    m_iSpeed     = src.m_iSpeed;
    m_dwBlocker  = src.m_dwBlocker;
    m_bOnWater   = src.m_bOnWater;

    return ( *this );
}

CMsgPlaceBldg::CMsgPlaceBldg( CHexCoord const& hex, int iDir, int iBldg ): _CMsgBldg( place_bldg )
{

    m_dwIDBldg = m_dwIDVeh = 0;
    m_hexBldg              = hex;
    m_iDir                 = iDir;
    m_iType                = iBldg;
    m_bShow                = FALSE;
    ASSERT_CMD( this );
}

CMsgPlaceVeh::CMsgPlaceVeh( CBuilding* pBldg, CHexCoord const& hexDest, int iPlyr, int iType ): _CMsgVeh( place_veh )
{

    m_dwID     = 0;
    m_dwIDBldg = pBldg->GetID( );
    if ( iPlyr == -1 )
        m_iPlyrNum = pBldg->GetOwner( )->GetPlyrNum( );
    else
        m_iPlyrNum = iPlyr;
    m_iType      = iType;
    m_hexDest    = hexDest;
    m_iRouteMode = CVehicle::cant_deploy;

    CSubHex _next;
    CVehicle::GetExitLoc( pBldg, iType, _next, m_ptHead, m_ptTail );

    ASSERT_CMD( this );
}

CMsgPlaceVeh::CMsgPlaceVeh( CHexCoord const& hex, CHexCoord const& hexDest, int iPlyr, int iType )
    : _CMsgVeh( place_veh )
{

    m_dwID     = 0;
    m_dwIDBldg = 0;
    if ( iPlyr == -1 )
        m_iPlyrNum = theGame.GetMe( )->GetPlyrNum( );
    else
        m_iPlyrNum = iPlyr;
    m_iType    = iType;
    m_ptHead.x = m_ptTail.x = hex.X( ) * 2;
    m_ptHead.y              = hex.Y( ) * 2;
    m_ptTail.y              = hex.Y( ) * 2 + 1;
    m_ptTail.Wrap( );
    m_hexDest    = hexDest;
    m_iRouteMode = CVehicle::stop;
    ASSERT_CMD( this );
}

CMsgVehNew::CMsgVehNew( CVehicle const* pVeh, DWORD dwIDBldg ): _CMsgVeh( veh_new )
{

    m_dwIDBldg   = dwIDBldg;
    m_dwID       = pVeh->GetID( );
    m_iPlyrNum   = pVeh->GetOwner( )->GetPlyrNum( );
    m_iType      = pVeh->GetData( )->GetType( );
    m_ptHead     = pVeh->GetPtHead( );
    m_ptTail     = pVeh->GetPtTail( );
    m_hexDest    = pVeh->GetHexDest( );
    m_iRouteMode = pVeh->GetRouteMode( );

    ASSERT_CMD( this );
}

CMsgBuildBldg::CMsgBuildBldg( CVehicle const* pVeh, CHexCoord const& hex, int iDir, int iBldg ): _CMsgBldg( build_bldg )
{

    ASSERT_VALID_OR_NULL( pVeh );

    m_dwIDBldg = 0;
    if ( pVeh == NULL )
    {
        TRAP( );
        m_iPlyrNum = 0;
        m_dwIDVeh  = 0;
        m_bShow    = FALSE;
    }
    else
    {
        m_iPlyrNum = pVeh->GetOwner( )->GetPlyrNum( );
        m_dwIDVeh  = pVeh->GetID( );
        if ( pVeh->GetOwner( )->IsMe( ) )
            m_bShow = theApp.IsShareware( );
        else
            m_bShow = FALSE;
    }

    m_hexBldg = hex;
    m_iDir    = iDir;
    m_iType   = iBldg;
    ASSERT_CMD( this );
}

CMsgBuildVeh::CMsgBuildVeh( CBuilding const* pBldg, int iVehType, int iNum ): CNetCmd( build_veh )
{

    ASSERT_VALID( pBldg );

    m_dwID     = pBldg->GetID( );
    m_iVehType = iVehType;
    m_iNum     = iNum;
    ASSERT_CMD( this );
}

CMsgBuildRoad::CMsgBuildRoad( CVehicle const* pVeh ): _CMsgRoad( build_road )
{

    ASSERT_VALID( pVeh );

    m_iPlyrNum = pVeh->GetOwner( )->GetPlyrNum( );
    m_dwID     = pVeh->GetID( );
    m_hexBuild = pVeh->GetHexHead( );
    m_iMode    = street;

    ASSERT_CMD( this );
}

CMsgBuildBridge::CMsgBuildBridge( CVehicle const* pVeh, CHexCoord const& hexEnd ): _CMsgBridge( build_bridge )
{

    ASSERT_VALID( pVeh );

    m_iPlyrNum = pVeh->GetOwner( )->GetPlyrNum( );
    m_dwIDVeh  = pVeh->GetID( );
    m_dwIDBrdg = 0;
    m_hexStart = pVeh->GetHexHead( );
    m_hexEnd   = hexEnd;
    m_iMode    = street;

    ASSERT_CMD( this );
}

CMsgVehGoto::CMsgVehGoto( CVehicle const* pVeh ): _CMsgVehGo( veh_goto )
{

    ASSERT_VALID_LOC( pVeh );

    m_dwID     = pVeh->GetID( );
    m_iPlyrNum = pVeh->GetOwner( )->GetPlyrNum( );
    m_ptDest   = pVeh->m_ptDest;
    m_ptNext   = pVeh->m_ptNext;
    m_ptHead   = pVeh->m_ptHead;
    m_ptTail   = pVeh->m_ptTail;
    m_hexNext  = pVeh->m_hexNext;
    m_hexDest  = pVeh->m_hexDest;
    m_iDir     = pVeh->m_iDir;

    if ( pVeh->GetTurret( ) )
        m_iTurretDir = pVeh->GetTurret( )->GetDir( );

    m_iXadd      = pVeh->m_iXadd;
    m_iYadd      = pVeh->m_iYadd;
    m_iDadd      = pVeh->m_iDadd;
    m_iTadd      = pVeh->m_iTadd;
    m_iMode      = pVeh->GetRouteMode( );
    m_iOwn       = pVeh->GetHexOwnership( );
    m_iStepsLeft = pVeh->m_iStepsLeft;
    m_iSpeed     = pVeh->m_iSpeed;

    m_bOnWater = pVeh->IsOnWater( );

    ASSERT_CMD( this );
}

void CMsgVehGoto::ToErr( CUnit const* pBlk )
{

    ASSERT( m_ptDest != m_ptHead );

    m_bMsg = err_veh_goto;

    if ( pBlk == NULL )
        m_dwBlocker = NULL;
    else
        m_dwBlocker = pBlk->GetID( );
}

CMsgTransMat::CMsgTransMat( CUnit const* pSrc, CUnit const* pDest ): CNetCmd( trans_mat )
{

    TRAP( );
    m_dwIDSrc  = pSrc->GetID( );
    m_dwIDDest = pDest->GetID( );
    memset( m_aiMat, 0, sizeof( m_aiMat ) );
}

CMsgUnitControl::CMsgUnitControl( CUnit const* pUnit, int iCmd ): CNetCmd( unit_control )
{

    ASSERT_VALID( pUnit );
    TRAP( );

    m_dwID = pUnit->GetID( );
    m_cCmd = iCmd;
    ASSERT_CMD( this );
}

CMsgUnitDamage::CMsgUnitDamage( CUnit const* pTarget, CUnit const* pShoot, CUnit const* pDamage, int iDamage )
    : CNetCmd( unit_damage )
{

    ASSERT_VALID_OR_NULL( pTarget );
    ASSERT_VALID( pShoot );
    ASSERT_VALID_OR_NULL( pDamage );

    if ( pTarget != NULL )
    {
        m_dwIDTarget  = pTarget->GetID( );
        m_iPlyrTarget = pTarget->GetOwner( )->GetPlyrNum( );
    }
    else
    {
        m_dwIDTarget  = 0;
        m_iPlyrTarget = -1;
    }

    m_dwIDShoot  = pShoot->GetID( );
    m_iPlyrShoot = pShoot->GetOwner( )->GetPlyrNum( );

    if ( pDamage != NULL )
    {
        m_dwIDDamage  = pDamage->GetID( );
        m_iPlyrDamage = pDamage->GetOwner( )->GetPlyrNum( );
    }
    else
    {
        m_dwIDDamage  = 0;
        m_iPlyrDamage = -1;
    }

    m_iDamageShot = iDamage;
}

CMsgUnitDamage::CMsgUnitDamage( CMsgCompUnitDamageElem* pElem )
{

    CUnit* pShoot  = ::GetUnit( pElem->m_dwKiller );
    CUnit* pDamage = ::GetUnit( pElem->m_dwID );

    if ( pShoot != NULL )
    {
        m_dwIDShoot  = pShoot->GetID( );
        m_iPlyrShoot = pShoot->GetOwner( )->GetPlyrNum( );
    }
    else
    {
        m_dwIDShoot  = 0;
        m_iPlyrShoot = -1;
    }

    if ( pDamage != NULL )
    {
        m_dwIDDamage  = pDamage->GetID( );
        m_iPlyrDamage = pDamage->GetOwner( )->GetPlyrNum( );
    }
    else
    {
        m_dwIDDamage  = 0;
        m_iPlyrDamage = -1;
    }

    m_dwIDTarget  = 0;
    m_iPlyrTarget = -1;
    m_iDamageShot = pElem->m_wDamage;
}

CMsgUnitSetDamage::CMsgUnitSetDamage( CMsgCompUnitDamageElem* pElem )
{

    m_dwIDDamage   = pElem->m_dwID;
    m_dwIDShoot    = pElem->m_dwKiller;
    m_iDamageLevel = pElem->m_wDamage;
}

CMsgUnitRepair::CMsgUnitRepair( CUnit const* pUnit, int iRepair ): CNetCmd( unit_repair )
{

    m_dwID         = pUnit->GetID( );
    m_iPlyr        = pUnit->GetOwner( )->GetPlyrNum( );
    m_iRepair      = iRepair;
    m_iDamageLevel = pUnit->GetDamagePoints( ) + iRepair;
}

CMsgBldgStat::CMsgBldgStat( CBuilding const* pBldg ): CNetCmd( bldg_stat )
{

    ASSERT_VALID( pBldg );

    m_dwID       = pBldg->GetID( );
    m_iPlyrNum   = pBldg->GetOwner( )->GetPlyrNum( );
    m_iBuilt     = pBldg->GetBuiltPer( );
    m_iConstDone = pBldg->GetConstDone( );
    m_iFoundPer  = pBldg->GetFoundPer( );
    m_iSkltnPer  = pBldg->GetSkltnPer( );
    m_iFinalPer  = pBldg->GetFinalPer( );
    m_iType      = pBldg->GetData( )->GetType( );
    m_iFlags     = 0;
    ASSERT_CMD( this );
}

CMsgVehStat::CMsgVehStat( CVehicle const* pVeh ): CNetCmd( veh_stat )
{

    ASSERT_VALID( pVeh );
    TRAP( );

    m_dwID     = pVeh->GetID( );
    m_iPlyrNum = pVeh->GetOwner( )->GetPlyrNum( );
    m_iDamage  = pVeh->GetDamagePer( );
    m_iVehType = pVeh->GetData( )->GetType( );
    m_iFlags   = 0;
    ASSERT_CMD( this );
}

CMsgVehDest::CMsgVehDest( CVehicle const* pVeh ): CNetCmd( veh_dest )
{

    ASSERT_VALID( pVeh );

    m_dwID     = pVeh->GetID( );
    m_iPlyrNum = pVeh->GetOwner( )->GetPlyrNum( );
    m_hexDest  = pVeh->GetHexDest( );
    ASSERT_CMD( this );
}

CMsgRoadDone::CMsgRoadDone( CVehicle const* pVeh ): _CMsgRoad( road_done )
{

    ASSERT_VALID( pVeh );

    m_iPlyrNum = pVeh->GetOwner( )->GetPlyrNum( );
    m_dwID     = pVeh->GetID( );
    m_hexBuild = pVeh->GetHexHead( );
    m_iMode    = street;
    ASSERT_CMD( this );
}

CMsgVehSetDest::CMsgVehSetDest( CVehicle const* pVeh ): CNetCmd( veh_set_dest )
{

    ASSERT_VALID( pVeh );
    TRAP( );

    m_dwID  = pVeh->GetID( );
    m_hex   = pVeh->GetHexDest( );
    m_sub   = pVeh->GetPtDest( );
    m_iSub  = pVeh->GetDestMode( );
    m_iMode = pVeh->GetRouteMode( );
    ASSERT_CMD( this );
}

CMsgVehSetDest::CMsgVehSetDest( DWORD dwID, CHexCoord const& hex, int iMode ): CNetCmd( veh_set_dest )
{

    m_dwID  = dwID;
    m_hex   = hex;
    m_sub   = CSubHex( hex.X( ) * 2, hex.Y( ) * 2 );
    m_iSub  = CVehicle::full;
    m_iMode = iMode;
    ASSERT_CMD( this );
}

CMsgVehSetDest::CMsgVehSetDest( DWORD dwID, CSubHex const& _sub, int iMode ): CNetCmd( veh_set_dest )
{

    m_dwID  = dwID;
    m_hex   = _sub;
    m_sub   = _sub;
    m_iSub  = CVehicle::sub;
    m_iMode = iMode;
    ASSERT_CMD( this );
}

CMsgDestroyUnit::CMsgDestroyUnit( CUnit const* pUnit ): CNetCmd( destroy_unit )
{

    ASSERT_VALID( pUnit );

    m_dwID = pUnit->GetID( );
    ASSERT_CMD( this );
}

CMsgDeleteUnit::CMsgDeleteUnit( CUnit const* pUnit, DWORD dwIDKiller ): CNetCmd( delete_unit )
{

    ASSERT_VALID( pUnit );

    m_dwID        = pUnit->GetID( );
    m_iPlyrNum    = pUnit->GetOwner( )->GetPlyrNum( );
    m_iPlyrKiller = dwIDKiller;
    ASSERT_CMD( this );
}

CMsgAttack::CMsgAttack( DWORD dwShooter, DWORD dwTarget ): CNetCmd( attack )
{

    m_dwShooter = dwShooter;
    m_dwTarget  = dwTarget;
}

CMsgDeployIt::CMsgDeployIt( CVehicle const* pVeh ): CNetCmd( deploy_it )
{

    m_dwID = pVeh->GetID( );
}

CMsgPlyrDying::CMsgPlyrDying( CPlayer const* pPlyr ): CNetCmd( plyr_dying )
{

    m_iPlyrNum = pPlyr->GetPlyrNum( );
}

CMsgPlyrDead::CMsgPlyrDead( CPlayer* pPlyr ): CNetCmd( plyr_dead )
{

    m_pPlyr = pPlyr;
}

CMsgLoadCarrier::CMsgLoadCarrier( CVehicle const* pCargo, CVehicle const* pCarrier ): CNetCmd( load_carrier )
{

    m_dwIDCargo   = pCargo->GetID( );
    m_dwIDCarrier = pCarrier->GetID( );
}

CMsgUnloadCarrier::CMsgUnloadCarrier( CVehicle const* pVeh ): CNetCmd( unload_carrier )
{

    m_dwID = pVeh->GetID( );
}

CMsgSeeUnit::CMsgSeeUnit( CUnit const* pMe, CUnit const* pSpot ): CNetCmd( see_unit )
{

    m_dwIDme     = pMe->GetID( );
    m_iPlyrNumMe = pMe->GetOwner( )->GetPlyrNum( );
    m_hexMe =
        ( pMe->GetUnitType( ) == CUnit::building ) ? ( (CBuilding*)pMe )->GetHex( ) : ( (CVehicle*)pMe )->GetHexHead( );

    m_dwIDspot     = pSpot->GetID( );
    m_iPlyrNumSpot = pSpot->GetOwner( )->GetPlyrNum( );
    m_hexSpot      = ( pSpot->GetUnitType( ) == CUnit::building ) ? ( (CBuilding*)pSpot )->GetHex( )
                                                                  : ( (CVehicle*)pSpot )->GetHexHead( );
}

CMsgUnitAttacked::CMsgUnitAttacked( CUnit const* pMe, CUnit const* pTarget ): CNetCmd( unit_attacked )
{

    m_dwIDme     = pMe->GetID( );
    m_iPlyrNumMe = pMe->GetOwner( )->GetPlyrNum( );
    m_hexMe =
        ( pMe->GetUnitType( ) == CUnit::building ) ? ( (CBuilding*)pMe )->GetHex( ) : ( (CVehicle*)pMe )->GetHexHead( );

    m_dwIDtarget     = pTarget->GetID( );
    m_iPlyrNumTarget = pTarget->GetOwner( )->GetPlyrNum( );
    m_hexTarget      = ( pTarget->GetUnitType( ) == CUnit::building ) ? ( (CBuilding*)pTarget )->GetHex( )
                                                                      : ( (CVehicle*)pTarget )->GetHexHead( );
}

CMsgBuildCiv::CMsgBuildCiv( CPlayer const* pPlyr, int iBldgNum ): CNetCmd( build_civ )
{

    m_iPlyrNum = pPlyr->GetPlyrNum( );
    m_iBldgNum = iBldgNum;
}

CMsgLoaded::CMsgLoaded( CVehicle const* pVehCarrier, CVehicle const* pCargo ): CNetCmd( unit_loaded )
{

    m_dwIDCarrier = pVehCarrier->GetID( );
    m_dwIDCargo   = pCargo->GetID( );
}

CMsgRepaired::CMsgRepaired( CVehicle const* pVeh ): CNetCmd( unit_repaired )
{

    m_dwIDVeh  = pVeh->GetID( );
    m_iPlyrNum = pVeh->GetOwner( )->GetPlyrNum( );
}

CMsgAiMsg* CMsgAiMsg::Alloc( CPlayer const* pPlyr, CNetCmd const* pMsg, int iLen )
{

    CMsgAiMsg* pRtn   = (CMsgAiMsg*)new char[iLen + sizeof( CMsgAiMsg )];
    pRtn->m_iAllocLen = iLen + sizeof( CMsgAiMsg );
    pRtn->m_bMsg      = CNetCmd::ai_msg;
    pRtn->m_iPlyrNum  = pPlyr->GetPlyrNum( );
    pRtn->m_iLen      = iLen;
    memcpy( pRtn + 1, pMsg, iLen );
    return ( pRtn );
}

CMsgScenario::CMsgScenario( ): CNetCmd( scenario )
{

    m_iScenario = theGame.GetScenario( );
    m_iPlyrHp   = theGame.GetMe( )->GetPlyrNum( );
}

CMsgScenarioAtk::CMsgScenarioAtk( CUnit const* pTarget, int iPlyrAtk, DWORD dwAtk, int iWght ): CNetCmd( scenario_atk )
{

    m_iPlyrHp    = theGame.GetMe( )->GetPlyrNum( );
    m_iPlyrAi    = iPlyrAtk;
    m_dwIDTarget = pTarget->GetID( );
    m_dwIDAtk    = dwAtk;
    m_iWeight    = iWght;
}

CMsgScenarioAtk::CMsgScenarioAtk( CUnit const* pTarget, CPlayer const* pAtk, int iWght ): CNetCmd( scenario_atk )
{

    TRAP( );
    m_iPlyrHp    = theGame.GetMe( )->GetPlyrNum( );
    m_iPlyrAi    = pAtk->GetPlyrNum( );
    m_dwIDTarget = pTarget->GetID( );
    m_dwIDAtk    = 0;
    m_iWeight    = iWght;
}

CMsgOutOfLos::CMsgOutOfLos( CUnit const* pAttacker, CUnit const* pTarget ): CNetCmd( out_of_LOS )
{

    m_dwIDAttacker  = pAttacker->GetID( );
    m_iPlyrAttacker = pAttacker->GetOwner( )->GetPlyrNum( );
    m_dwIDTarget    = pTarget->GetID( );
    m_iPlyrTarget   = pTarget->GetOwner( )->GetPlyrNum( );
};

CMsgSetRelations::CMsgSetRelations( CPlayer const* pPlyrSet, CPlayer const* pPlyrGet, int iLevel )
    : CNetCmd( set_relations )
{

    m_iPlyrNumSet = pPlyrSet->GetPlyrNum( );
    m_iPlyrNumGet = pPlyrGet->GetPlyrNum( );
    m_iLevel      = iLevel;
}

CMsgBldgMat::CMsgBldgMat( CUnit* pUnit, BOOL bTotal ): CNetCmd( bldg_materials )
{

    m_dwID = pUnit->GetID( );

    for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ )
        if ( bTotal )
            m_aiMat[iInd] = pUnit->GetStore( iInd );
        else
            m_aiMat[iInd] = pUnit->GetStoreDiff( iInd, TRUE );

    m_bTotal = bTotal;
}

CMsgGameSpeed::CMsgGameSpeed( int iSpeed ): CNetCmd( game_speed )
{

    if ( theGame.HaveHP( ) )
        m_iPlyrNum = theGame.GetMe( )->GetPlyrNum( );
    else
        m_iPlyrNum = -1;
    m_iSpeed = iSpeed;
}

CMsgNewServer::CMsgNewServer( CPlayer const* pPlyr ): CNetCmd( cmd_new_server )
{

    m_iPlyrNum = pPlyr->GetPlyrNum( );
}

CMsgGiveUnit::CMsgGiveUnit( CUnit const* pUnit, CPlayer const* pPlyr ): CNetCmd( give_unit )
{

    m_iPlyrNum = pPlyr->GetPlyrNum( );
    m_dwID     = pUnit->GetID( );
    for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ ) m_aiStore[iInd] = pUnit->GetStore( iInd );
}

CMsgCancelLoad::CMsgCancelLoad( CPlayer* pPlyr ): CNetCmd( cancel_load )
{

    m_iNetNum = pPlyr->GetNetNum( );
}

CMsgPauseMsg::CMsgPauseMsg( BOOL bPause ): CNetCmd( pause_messages )
{

    if ( theGame.HaveHP( ) )
        m_iPlyrNum = theGame.GetMe( )->GetPlyrNum( );
    else
        m_iPlyrNum = 0;
    m_bPause = bPause;
}

int CMsgVehCompLoc::AddVeh( CVehicle* pVeh )
{

    m_aMVCLE[m_iNumMsgs] = *pVeh;
    m_iNumMsgs++;
    return m_iNumMsgs;
}

CMsgVehLoc::CMsgVehLoc( CMsgVehCompLocElem const* pElem ): _CMsgVehGo( veh_loc )
{

    m_ptDest     = CSubHex( pElem->m_wDestX, pElem->m_wDestY );
    m_ptNext     = CSubHex( pElem->m_wNextX, pElem->m_wNextY );
    m_ptHead     = CSubHex( pElem->m_wHeadX, pElem->m_wHeadY );
    m_ptTail     = CSubHex( pElem->m_wTailX, pElem->m_wTailY );
    m_hexNext    = CSubHex( pElem->m_wHexNextX, pElem->m_wHexNextY );
    m_hexDest    = CSubHex( pElem->m_wHexDestX, pElem->m_wHexDestY );
    m_dwID       = pElem->m_dwID;
    m_iPlyrNum   = 0;
    m_iDir       = pElem->m_bDir;
    m_iTurretDir = pElem->m_bTurretDir;
    m_iXadd      = pElem->m_cXadd;
    m_iYadd      = pElem->m_cYadd;
    m_iDadd      = pElem->m_cDadd;
    m_iTadd      = pElem->m_cTadd;
    m_iMode      = pElem->m_bMode;
    m_iStepsLeft = pElem->m_bStepsLeft;
    m_iSpeed     = pElem->m_bSpeed;

    m_iOwn     = pElem->m_bFlags & CMsgVehCompLocElem::_own;
    m_bOnWater = pElem->m_bFlags & CMsgVehCompLocElem::_on_water;
}

const CMsgVehCompLocElem CMsgVehCompLocElem::operator=( CVehicle const& src )
{

    if ( src.GetTurret( ) )
        TRAP( ( src.GetTurret( )->GetDir( ) < 0 ) || ( 256 <= src.GetTurret( )->GetDir( ) ) );
    TRAP( ( src.m_iDir < 0 ) || ( 256 <= src.m_iDir ) );
    TRAP( ( src.m_iXadd < -127 ) || ( 127 <= src.m_iXadd ) );
    TRAP( ( src.m_iYadd < -127 ) || ( 127 <= src.m_iYadd ) );
    TRAP( ( src.m_iDadd < -127 ) || ( 127 <= src.m_iDadd ) );
    TRAP( ( src.m_iTadd < -127 ) || ( 127 <= src.m_iTadd ) );
    TRAP( ( src.m_iDir < 0 ) || ( 256 <= src.m_iDir ) );
    TRAP( ( src.m_iStepsLeft < 0 ) || ( 256 <= src.m_iStepsLeft ) );
    TRAP( ( src.m_iSpeed < 0 ) || ( 256 <= src.m_iSpeed ) );

    m_dwID      = src.GetID( );
    m_wDestX    = (WORD)src.m_ptDest.x;
    m_wDestY    = (WORD)src.m_ptDest.y;
    m_wNextX    = (WORD)src.m_ptNext.x;
    m_wNextY    = (WORD)src.m_ptNext.y;
    m_wHeadX    = (WORD)src.m_ptHead.x;
    m_wHeadY    = (WORD)src.m_ptHead.y;
    m_wTailX    = (WORD)src.m_ptTail.x;
    m_wTailY    = (WORD)src.m_ptTail.y;
    m_wHexNextX = (WORD)src.m_hexNext.X( );
    m_wHexNextY = (WORD)src.m_hexNext.Y( );
    m_wHexDestX = (WORD)src.m_hexDest.X( );
    m_wHexDestY = (WORD)src.m_hexDest.Y( );
    m_bDir      = (BYTE)src.m_iDir;
    if ( src.GetTurret( ) )
        m_bTurretDir = (BYTE)src.GetTurret( )->GetDir( );
    m_cXadd      = (signed char)src.m_iXadd;
    m_cYadd      = (signed char)src.m_iYadd;
    m_cDadd      = (signed char)src.m_iDadd;
    m_cTadd      = (signed char)src.m_iTadd;
    m_bMode      = (BYTE)src.GetRouteMode( );
    m_bStepsLeft = (BYTE)src.m_iStepsLeft;
    m_bSpeed     = (BYTE)src.m_iSpeed;

    m_bFlags = src.GetHexOwnership( ) ? _own : 0;
    if ( src.IsOnWater( ) )
        m_bFlags |= _on_water;

    return ( *this );
}

int CMsgCompUnitDamage::AddUnit( CUnit* pUnit, int iDam )
{

    m_aMCUDE[m_iNumMsgs].m_dwID     = pUnit->GetID( );
    m_aMCUDE[m_iNumMsgs].m_dwKiller = pUnit->GetLastShooter( );
    m_aMCUDE[m_iNumMsgs].m_wDamage  = iDam;
    m_iNumMsgs++;
    return m_iNumMsgs;
}

int CMsgCompUnitSetDamage::AddUnit( CUnit* pUnit )
{

    m_aMCUDE[m_iNumMsgs].m_dwID     = pUnit->GetID( );
    m_aMCUDE[m_iNumMsgs].m_dwKiller = pUnit->GetLastShooter( );
    m_aMCUDE[m_iNumMsgs].m_wDamage  = pUnit->GetDamagePoints( );
    m_iNumMsgs++;
    return m_iNumMsgs;
}

int CMsgShoot::AddShot( CUnit const* pUnit, CUnit const* pTarget, CMapLoc const& mlDest, int iShots )
{

    m_aMSE[m_iNumMsgs].m_dwID       = pUnit->GetID( );
    m_aMSE[m_iNumMsgs].m_dwIDTarget = pTarget->GetID( );
    m_aMSE[m_iNumMsgs].m_wDestX     = mlDest.x;
    m_aMSE[m_iNumMsgs].m_wDestY     = mlDest.y;
    m_aMSE[m_iNumMsgs].m_wNumShots  = (WORD)iShots;

    m_iNumMsgs++;
    return m_iNumMsgs;
}

CNetAiGpf::CNetAiGpf( CPlayer const* pPlyr ): CNetCmd( ai_gpf_takeover )
{

    TRAP( );
    m_iPlyrNum = pPlyr->GetPlyrNum( );
}

CNetRsrchDisc::CNetRsrchDisc( CPlayer const* pPlyr, int iRsrch ): CNetCmd( research_disc )
{

    m_iPlyrNum = pPlyr->GetPlyrNum( );
    m_iRsrch   = pPlyr->GetRsrchItem( );
}

CNetNeedSaveInfo::CNetNeedSaveInfo( CPlayer const* pPlyr ): CNetCmd( need_save_info )
{

    m_iPlyrNum = pPlyr->GetPlyrNum( );
}

CNetSaveInfo::CNetSaveInfo( CPlayer const* pPlyr ): CNetCmd( save_info )
{

    m_iPlyrNum   = pPlyr->GetPlyrNum( );
    m_iPplBldg   = pPlyr->GetPplBldg( );
    m_iPplVeh    = pPlyr->GetPplVeh( );
    m_iFood      = pPlyr->GetFood( );
    m_iGas       = pPlyr->GetGasHave( );
    m_iRsrchItem = pPlyr->GetRsrchItem( );
    if ( m_iRsrchItem > 0 )
    {
        TRAP( );
        m_iPtsDiscovered = ( ( (CPlayer*)pPlyr )->GetRsrch( m_iRsrchItem ) ).m_iPtsDiscovered;
    }
}


#ifdef _DEBUG
void CNetReady::AssertValid( ) const
{

    ASSERT( m_bMsg == cmd_ready );
    ASSERT( m_iPlyrNum > 0 );
}

void CNetYouAre::AssertValid( ) const
{

    ASSERT( m_bMsg == cmd_you_are );
}

void CNetPlayer::AssertValid( ) const
{

    ASSERT( m_bMsg == cmd_player );
    ASSERT( (unsigned)m_iLen >= sizeof( CNetPlayer ) + strlen( m_sName ) + 1 );
}

void CNetStart::AssertValid( ) const
{

    ASSERT( m_bMsg == cmd_start );
}

void _CMsgVeh::AssertValid( ) const
{

    if ( ( m_dwID != 0 ) && ( theGame.AmServer( ) ) )
        ASSERT( ( 0 < m_dwID ) && ( m_dwID < theGame.GetNextID( ) ) );
    ASSERT( ( 0 <= m_iType ) && ( m_iType < theTransports.GetNumTransports( ) ) );
    ASSERT_VALID_STRUCT( &m_ptHead );
    ASSERT_VALID_STRUCT( &m_ptTail );
    ASSERT_HEX_COORD( &m_hexDest );
    ASSERT( ( 0 <= m_iRouteMode ) && ( m_iRouteMode < CVehicle::num_mode ) );
}

void _CMsgBldg::AssertValid( ) const
{

    if ( ( m_dwIDBldg != 0 ) && ( theGame.AmServer( ) ) )
        ASSERT( ( 0 < m_dwIDBldg ) && ( m_dwIDBldg < theGame.GetNextID( ) ) );
    if ( ( m_dwIDVeh != 0 ) && ( theGame.AmServer( ) ) )
        ASSERT( ( 0 < m_dwIDVeh ) && ( m_dwIDVeh < theGame.GetNextID( ) ) );
    ASSERT_HEX_COORD( &m_hexBldg );
    ASSERT( ( 0 <= m_iDir ) && ( m_iDir <= 3 ) );
    ASSERT( ( 0 <= m_iType ) && ( m_iType < theStructures.GetNumBuildings( ) ) );
}

void _CMsgRoad::AssertValid( ) const
{

    if ( theGame.AmServer( ) )
        ASSERT( ( 0 < m_dwID ) && ( m_dwID < theGame.GetNextID( ) ) );
    ASSERT( ( m_iMode == one_hex ) || ( m_iMode == street ) );
    ASSERT_HEX_COORD( &m_hexBuild );
}

void _CMsgVehGo::AssertValid( ) const
{

    CVehicle* pVeh = theVehicleMap.GetVehicle( m_dwID );
    ASSERT( pVeh != NULL );
    ASSERT( m_iPlyrNum == pVeh->GetOwner( )->GetPlyrNum( ) );
    ASSERT_HEX_COORD( &m_hexNext );
    ASSERT_HEX_COORD( &m_hexDest );
    ASSERT_VALID_STRUCT( &m_ptNext );
    ASSERT_VALID_STRUCT( &m_ptHead );
    ASSERT_VALID_STRUCT( &m_ptTail );
    ASSERT( m_hexDest.SameHex( m_ptDest ) );
}

#ifdef BUGBUG
void _CMsgDamage::AssertValid( ) const
{

    if ( theGame.AmServer( ) )
    {
        ASSERT( ( 0 <= m_dwIDDamage ) && ( m_dwIDDamage < theGame.GetNextID( ) ) );
        ASSERT( ( 0 <= m_dwIDShoot ) && ( m_dwIDShoot < theGame.GetNextID( ) ) );
    }
    ASSERT( ( ::_GetUnit( m_dwIDTarget ) == NULL ) ||
            ( m_iPlyrTarget == ::_GetUnit( m_dwIDTarget )->GetOwner( )->GetPlyrNum( ) ) );
    ASSERT( ( ::_GetUnit( m_dwIDDamage ) == NULL ) ||
            ( m_iPlyrDamage == ::_GetUnit( m_dwIDDamage )->GetOwner( )->GetPlyrNum( ) ) );
    ASSERT( ( ::_GetUnit( m_dwIDShoot ) == NULL ) ||
            ( m_iPlyrShoot == ::_GetUnit( m_dwIDShoot )->GetOwner( )->GetPlyrNum( ) ) );

    CUnit* pShoot = ::_GetUnit( m_dwIDShoot );
    if ( pShoot != NULL )
    {
        int x = CMapLoc::Diff( pShoot->GetMapLoc( ).x - m_mlDest.x );
#ifndef _GG
        ASSERT( ( -MAX_HEX_HT * 16 < x ) && ( x < MAX_HEX_HT * 16 ) );
#endif
        int y = CMapLoc::Diff( pShoot->GetMapLoc( ).y - m_mlDest.y );
        ASSERT( ( -MAX_HEX_HT * 16 < y ) && ( y < MAX_HEX_HT * 16 ) );
    }
}
#endif

void CMsgPlaceBldg::AssertValid( ) const
{

    ASSERT( ( m_bMsg == place_bldg ) || ( m_bMsg == err_place_bldg ) );

    _CMsgBldg::AssertValid( );
}

void CMsgPlaceVeh::AssertValid( ) const
{

    ASSERT( m_bMsg == place_veh );

    _CMsgVeh::AssertValid( );
}

void CMsgBuildBldg::AssertValid( ) const
{

    ASSERT( ( m_bMsg == build_bldg ) || ( m_bMsg == err_build_bldg ) );
    _CMsgBldg::AssertValid( );

    if ( theGame.AmServer( ) )
        ASSERT( ( 0 <= m_dwIDBldg ) && ( m_dwIDBldg < theGame.GetNextID( ) ) );
}

void CMsgBuildVeh::AssertValid( ) const
{

    ASSERT( m_bMsg == build_veh );
    if ( theGame.AmServer( ) )
        ASSERT( ( 0 < m_dwID ) && ( m_dwID < theGame.GetNextID( ) ) );
    ASSERT( ( 0 <= m_iVehType ) && ( m_iVehType < theTransports.GetNumTransports( ) ) );
}

void CMsgBuildRoad::AssertValid( ) const
{

    ASSERT( m_bMsg == build_road );

    _CMsgRoad::AssertValid( );
}

void CMsgVehGoto::AssertValid( ) const
{

    ASSERT( ( m_bMsg == veh_goto ) || ( m_bMsg == err_veh_goto ) || ( m_bMsg == err_veh_traffic ) );

    _CMsgVehGo::AssertValid( );
}

void CMsgVehLoc::AssertValid( ) const
{

    ASSERT( m_bMsg == veh_loc );

    _CMsgVehGo::AssertValid( );
}

void CMsgTransMat::AssertValid( ) const
{

    ASSERT( m_bMsg == trans_mat );
    if ( theGame.AmServer( ) )
    {
        ASSERT( ( 0 < m_dwIDSrc ) && ( m_dwIDSrc < theGame.GetNextID( ) ) );
        ASSERT( ( 0 < m_dwIDDest ) && ( m_dwIDDest < theGame.GetNextID( ) ) );
    }
}

void CMsgUnitControl::AssertValid( ) const
{

    ASSERT( m_bMsg == unit_control );
    if ( theGame.AmServer( ) )
        ASSERT( ( 0 < m_dwID ) && ( m_dwID < theGame.GetNextID( ) ) );
    ASSERT( ( 0 <= m_cCmd ) && ( m_cCmd < num_cmds ) );
}

void CMsgUnitDamage::AssertValid( ) const
{

    // BUGBUG	_CMsgDamage::AssertValid ();

    ASSERT( m_bMsg == unit_damage );
}

void CMsgBldgNew::AssertValid( ) const
{

    ASSERT( m_bMsg == bldg_new );
    // BUGBUG	_CMsgBldg::AssertValid ();

    if ( theGame.AmServer( ) )
        ASSERT( ( 0 <= m_dwIDBldg ) && ( m_dwIDBldg < theGame.GetNextID( ) ) );
}

void CMsgBldgStat::AssertValid( ) const
{

    ASSERT( m_bMsg == bldg_stat );
    if ( theGame.AmServer( ) )
        ASSERT( ( 0 < m_dwID ) && ( m_dwID < theGame.GetNextID( ) ) );
    ASSERT( m_iPlyrNum == theBuildingMap.GetBldg( m_dwID )->GetOwner( )->GetPlyrNum( ) );
    ASSERT( ( 0 <= m_iBuilt ) && ( m_iBuilt <= 100 ) );
}

void CMsgVehNew::AssertValid( ) const
{

    ASSERT( m_bMsg == veh_new );

    _CMsgVeh::AssertValid( );
}

void CMsgVehStat::AssertValid( ) const
{

    ASSERT( m_bMsg == veh_stat );
    if ( theGame.AmServer( ) )
        ASSERT( ( 0 < m_dwID ) && ( m_dwID < theGame.GetNextID( ) ) );
    ASSERT( m_iPlyrNum == theVehicleMap.GetVehicle( m_dwID )->GetOwner( )->GetPlyrNum( ) );
    ASSERT( ( 0 <= m_iDamage ) && ( m_iDamage <= 100 ) );
    ASSERT( m_iFlags != 0 );
}

void CMsgVehDest::AssertValid( ) const
{

    ASSERT( m_bMsg == veh_dest );
    if ( theGame.AmServer( ) )
        ASSERT( ( 0 < m_dwID ) && ( m_dwID < theGame.GetNextID( ) ) );
    ASSERT( m_iPlyrNum == theVehicleMap.GetVehicle( m_dwID )->GetOwner( )->GetPlyrNum( ) );
    ASSERT_HEX_COORD( &m_hexDest );
}

void CMsgRoadNew::AssertValid( ) const
{

    ASSERT( m_bMsg == road_new );

    _CMsgRoad::AssertValid( );
}

void CMsgRoadDone::AssertValid( ) const
{

    ASSERT( m_bMsg == road_done );

    _CMsgRoad::AssertValid( );
}

void CMsgUnitSetDamage::AssertValid( ) const
{

    // BUGBUG	_CMsgDamage::AssertValid ();

    ASSERT( m_bMsg == unit_set_damage );
}

void CMsgVehSetDest::AssertValid( ) const
{

    ASSERT( m_bMsg == veh_set_dest );
    if ( theGame.AmServer( ) )
        ASSERT( ( 0 < m_dwID ) && ( m_dwID < theGame.GetNextID( ) ) );
    ASSERT_HEX_COORD( &m_hex );
    ASSERT_VALID_STRUCT( &m_sub );
}

void CMsgDestroyUnit::AssertValid( ) const
{

    ASSERT( ( m_bMsg == destroy_unit ) || ( m_bMsg == unit_destroying ) || ( m_bMsg == stop_destroy_unit ) ||
            ( m_bMsg == stop_unit_destroying ) );
    if ( theGame.AmServer( ) )
        ASSERT( ( 0 < m_dwID ) && ( m_dwID < theGame.GetNextID( ) ) );
}

void CMsgDeleteUnit::AssertValid( ) const
{

    ASSERT( m_bMsg == delete_unit );
    if ( theGame.AmServer( ) )
        ASSERT( ( 0 < m_dwID ) && ( m_dwID < theGame.GetNextID( ) ) );
    CUnit* pUnit = ::_GetUnit( m_dwID );
    ASSERT( ( pUnit == NULL ) || ( m_iPlyrNum == pUnit->GetOwner( )->GetPlyrNum( ) ) );
}

void CMsgAttack::AssertValid( ) const
{

    ASSERT( m_bMsg == attack );
}

void CMsgUnitAttacked::AssertValid( ) const
{

    ASSERT( m_bMsg == unit_attacked );

    if ( theGame.AmServer( ) )
    {
        ASSERT( ( 0 < m_dwIDme ) && ( m_dwIDme < theGame.GetNextID( ) ) );
        ASSERT( ( 0 < m_dwIDtarget ) && ( m_dwIDtarget < theGame.GetNextID( ) ) );
    }
    ASSERT_HEX_COORD( &m_hexMe );
    ASSERT_HEX_COORD( &m_hexTarget );
}

void CMsgSeeUnit::AssertValid( ) const
{

    ASSERT( m_bMsg == see_unit );

    if ( theGame.AmServer( ) )
    {
        ASSERT( ( 0 < m_dwIDme ) && ( m_dwIDme < theGame.GetNextID( ) ) );
        ASSERT( ( 0 < m_dwIDspot ) && ( m_dwIDspot < theGame.GetNextID( ) ) );
    }
    ASSERT_HEX_COORD( &m_hexMe );
    ASSERT_HEX_COORD( &m_hexSpot );
}

void CNetCmd::AssertMsgValid( ) const
{

    switch ( m_bMsg )
    {
    case cmd_ready:
        ( (CNetReady*)this )->AssertValid( );
        break;
    case cmd_you_are:
        ( (CNetYouAre*)this )->AssertValid( );
        break;
    case cmd_player:
        ( (CNetPlayer*)this )->AssertValid( );
        break;
    case cmd_start:
        ( (CNetStart*)this )->AssertValid( );
        break;

    case place_veh:
        ( (CMsgPlaceVeh*)this )->AssertValid( );
        break;
    case veh_new:
        ( (CMsgVehNew*)this )->AssertValid( );
        break;

    case bldg_new:
        ( (CMsgBldgNew*)this )->AssertValid( );
        break;
    case place_bldg:
    case err_place_bldg:
        ( (CMsgPlaceBldg*)this )->AssertValid( );
        break;
    case build_bldg:
    case err_build_bldg:
        ( (CMsgBuildBldg*)this )->AssertValid( );
        break;

    case build_veh:
    case err_build_veh:
        ( (CMsgBuildVeh*)this )->AssertValid( );
        break;
    case veh_loc:
        ( (CMsgVehLoc*)this )->AssertValid( );
        break;
    case veh_goto:
    case err_veh_goto:
    case err_veh_traffic:
        ( (CMsgVehGoto*)this )->AssertValid( );
        break;

    case trans_mat:
        ( (CMsgTransMat*)this )->AssertValid( );
        break;

    case bldg_stat:
        ( (CMsgBldgStat*)this )->AssertValid( );
        break;
    case veh_stat:
        ( (CMsgVehStat*)this )->AssertValid( );
        break;
    case veh_dest:
        ( (CMsgVehDest*)this )->AssertValid( );
        break;
    case veh_set_dest:
        ( (CMsgVehSetDest*)this )->AssertValid( );
        break;

    case build_road:
    case err_build_road:
        ( (CMsgBuildRoad*)this )->AssertValid( );
        break;
    case road_new:
        ( (CMsgRoadNew*)this )->AssertValid( );
        break;
    case road_done:
        ( (CMsgRoadDone*)this )->AssertValid( );
        break;

    case unit_damage:
        ( (CMsgUnitDamage*)this )->AssertValid( );
        break;
    case unit_set_damage:
        ( (CMsgUnitSetDamage*)this )->AssertValid( );
        break;

    case destroy_unit:
    case unit_destroying:
    case stop_destroy_unit:
    case stop_unit_destroying:
        ( (CMsgDestroyUnit*)this )->AssertValid( );
        break;

    case delete_unit:
        ( (CMsgDeleteUnit*)this )->AssertValid( );
        break;

    case attack:
        ( (CMsgAttack*)this )->AssertValid( );
        break;
    case unit_attacked:
        ( (CMsgUnitAttacked*)this )->AssertValid( );
        break;
    case see_unit:
        ( (CMsgSeeUnit*)this )->AssertValid( );
        break;

    case ipc_msg:
    case deploy_it:
    case set_rsrch:
    case load_carrier:
    case unload_carrier:
    case cmd_init_done:
    case cmd_play:
    case cmd_plyr_status:
    case cmd_to_ai:
    case cmd_to_hp:
    case unit_control:
    case cmd_chat:
    case cmd_enum_plyrs:
    case cmd_plyr_join:
    case cmd_select_plyr:
    case cmd_select_ok:
    case cmd_select_not_ok:
    case cmd_plyr_taken:
    case repair_veh:
    case repair_bldg:
    case ai_msg:
    case cmd_pause:
    case cmd_resume:
    case plyr_dying:
    case plyr_dead:
    case scenario:
    case scenario_atk:
    case unit_loaded:
    case unit_repaired:
    case unit_repair:
    case unit_set_repair:
    case build_civ:
    case out_of_LOS:
    case build_bridge:
    case bridge_new:
    case bridge_done:
    case err_build_bridge:
    case set_relations:
    case shoot_gun:
    case set_time:
    case start_file:
    case ai_gpf_takeover:
    case need_save_info:
    case save_info:
    case research_disc:
        break;

    default:
        
       // ASSERT( FALSE );

        // 85 is veh_comp_loc
#ifdef LOGGINGON
       int  value = m_bMsg;
       char buf[64];
       sprintf( buf, "Unhandled event! %d\n", value );
       OutputDebugStringA( buf );
       
#endif
        break;
    }
}

#endif
