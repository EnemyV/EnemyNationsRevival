////////////////////////////////////////////////////////////////////////////
//
//  CAIUnit.cpp :  CAIUnit object implementation
//                 Divide and Conquer AI
//
//  Last update:    09/19/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "caiunit.hpp"

#include "caidata.hpp"
#include "logging.h"  // dave's logging system
#include "stdafx.h"


#if THREADS_ENABLED
extern CException* pException;  // standard exception for yielding
#endif

extern CAIData*         pGameData;  // pointer to game data interface
extern CRITICAL_SECTION cs;         // used by threads

#define new DEBUG_NEW

CAIUnit::CAIUnit( DWORD dwID, int iOwner, int iType, int iTypeUnit )
{
    m_dwID      = dwID;
    m_iOwner    = iOwner;
    m_iType     = iType;      // is either CUnit::vehicle or CUnit::building
    m_iTypeUnit = iTypeUnit;  // type of building or vehicle

    m_bControl   = TRUE;
    m_wGoal      = FALSE;
    m_wTask      = FALSE;
    m_dwData     = 0;
    m_wStatus    = 0;
    m_plCopyData = NULL;
    m_pwaParams  = NULL;
    m_pdwaParams = NULL;

    ASSERT_VALID( this );

    try
    {
        m_pwaParams = new WORD[CAI_SIZE];
        for ( int i = 0; i < CAI_SIZE; ++i ) m_pwaParams[i] = 0;
    }
    catch ( CException* e )
    {
        if ( m_pwaParams != NULL )
        {
            delete[] m_pwaParams;
            m_pwaParams = NULL;
        }
        throw( ERR_CAI_BAD_NEW );
    }

    try
    {
        m_pdwaParams = new DWORD[CAI_SIZE];
        for ( int i = 0; i < CAI_SIZE; ++i ) m_pdwaParams[i] = 0;
    }
    catch ( CException* e )
    {
        if ( m_pdwaParams != NULL )
        {
            delete[] m_pdwaParams;
            m_pdwaParams = NULL;
        }
        throw( ERR_CAI_BAD_NEW );
    }


    try
    {
        m_plCopyData = new CAICopyList( );
    }
    catch ( CException* e )
    {
        if ( m_plCopyData != NULL )
        {
            delete m_plCopyData;
            m_plCopyData = NULL;
        }
        throw( ERR_CAI_BAD_NEW );
    }
}

//
// the Update() suite of functions will copy the data
// from the various game data class objects into CAICopy
// objects and store them in the CAICopyList of this unit
//
void CAIUnit::Update( CUnitData const* pData )
{
    ASSERT_VALID( this );
    ASSERT_VALID( m_plCopyData );
    ASSERT_VALID( pData );

    // get current pointer to data copy or create a new one
    CAICopy* pCopy = m_plCopyData->GetCopy( CAICopy::CUnitData );
    if ( pCopy == NULL )
    {
        try
        {
            pCopy = new CAICopy( CAICopy::CUnitData );
            m_plCopyData->AddTail( (CObject*)pCopy );
        }
        catch ( CException* e )
        {
            if ( pCopy != NULL )
            {
                delete pCopy;
                pCopy = NULL;
            }
            throw( ERR_CAI_BAD_NEW );
        }
    }

    // now update the current data copy from the game data
    pCopy->m_aiDataIn[CAI_PEOPLE] = pData->GetPeople( );
    //	pCopy->m_aiDataIn[CAI_MAXMATERIAL] = pData->GetMaxMaterials();
    //	pCopy->m_aiDataIn[CAI_SPOTTING] = pData->GetSpottingRange();
    pCopy->m_aiDataIn[CAI_DAMAGEPTS] = pData->GetDamagePoints( );
    //	pCopy->m_aiDataIn[CAI_INITIATIVE] = pData->GetInitiative();
    //	pCopy->m_aiDataIn[CAI_RANGE] = pData->GetRange();
    //	pCopy->m_aiDataIn[CAI_SOFTATTACK] = pData->GetAttack(CUnitData::soft);
    //	pCopy->m_aiDataIn[CAI_HARDATTACK] = pData->GetAttack(CUnitData::hard);
    //	pCopy->m_aiDataIn[CAI_NAVALATTACK] = pData->GetAttack(CUnitData::naval);
    pCopy->m_aiDataIn[CAI_TARGETTYPE] = pData->GetTargetType( );
    //	pCopy->m_aiDataIn[CAI_GRDDEFENSE] = pData->GetGroundDefense();
    //	pCopy->m_aiDataIn[CAI_CLSDEFENSE] = pData->GetCloseDefense();
    //	pCopy->m_aiDataIn[CAI_FIRERATE] = pData->GetFireRate();
    //	pCopy->m_aiDataIn[CAI_ACCURACY] = pData->GetAccuracy();
}

void CAIUnit::Update( CBuildMaterials const* pData )
{
    ASSERT_VALID( this );
    ASSERT_VALID( m_plCopyData );
    ASSERT_VALID( pData );

    // get current pointer to data copy or create a new one
    CAICopy* pCopy = m_plCopyData->GetCopy( CAICopy::CBuildMaterials );
    if ( pCopy == NULL )
    {
        try
        {
            pCopy = new CAICopy( CAICopy::CBuildMaterials );
            m_plCopyData->AddTail( (CObject*)pCopy );
        }
        catch ( CException* e )
        {
            if ( pCopy != NULL )
            {
                delete pCopy;
                pCopy = NULL;
            }
            throw( ERR_CAI_BAD_NEW );
        }
    }
    // first clear out data slots
    for ( int i = 0; i < CAI_DATA_SLOTS; ++i )
    {
        pCopy->m_aiDataIn[i]  = 0;
        pCopy->m_aiDataOut[i] = 0;
    }

    // now update the current data copy from the game data
    for ( int i = 0; i < CMaterialTypes::num_types; ++i )
    {
        pCopy->m_aiDataIn[i]  = pData->GetInput( i );
        pCopy->m_aiDataOut[i] = pData->GetOutput( i );
    }
    // BUGBUG type appears to be gone
    // pCopy->m_aiDataIn[CAI_TYPEBUILD] = pData->GetType();
    pCopy->m_aiDataOut[CAI_TIMEBUILD] = pData->GetTime( );
}


void CAIUnit::Update( CBuildMine const* pData )
{
    ASSERT_VALID( this );
    ASSERT_VALID( m_plCopyData );
    ASSERT_VALID( pData );

    // get current pointer to data copy or create a new one
    CAICopy* pCopy = m_plCopyData->GetCopy( CAICopy::CBuildMine );
    if ( pCopy == NULL )
    {
        try
        {
            pCopy = new CAICopy( CAICopy::CBuildMine );
            m_plCopyData->AddTail( (CObject*)pCopy );
        }
        catch ( CException* e )
        {
            if ( pCopy != NULL )
            {
                delete pCopy;
                pCopy = NULL;
            }
            throw( ERR_CAI_BAD_NEW );
        }
    }
    // time to mine some quantity?
    pCopy->m_aiDataOut[CAI_TIMEBUILD] = pData->GetTimeToMine( );
    // material mined
    pCopy->m_aiDataOut[CAI_TYPEBUILD] = pData->GetTypeMines( );
}


void CAIUnit::Update( CBuildFarm const* pData )
{
    ASSERT_VALID( this );
    ASSERT_VALID( m_plCopyData );
    ASSERT_VALID( pData );

    // get current pointer to data copy or create a new one
    CAICopy* pCopy = m_plCopyData->GetCopy( CAICopy::CBuildFarm );
    if ( pCopy == NULL )
    {
        try
        {
            pCopy = new CAICopy( CAICopy::CBuildFarm );
            m_plCopyData->AddTail( (CObject*)pCopy );
        }
        catch ( CException* e )
        {
            if ( pCopy != NULL )
            {
                delete pCopy;
                pCopy = NULL;
            }
            throw( ERR_CAI_BAD_NEW );
        }
    }
    // time to farm some quantity?
    pCopy->m_aiDataOut[CAI_TIMEBUILD] = pData->GetTimeToFarm( );
    // quantity produced in that time
    pCopy->m_aiDataOut[CAI_PRODUCES] = pData->GetQuantity( );
    // material farmed
    pCopy->m_aiDataOut[CAI_TYPEBUILD] = pData->GetTypeFarm( );
}


void CAIUnit::Update( CBuildUnit const* pData )
{
    ASSERT_VALID( this );
    ASSERT_VALID( m_plCopyData );
    ASSERT_VALID( pData );

    // get current pointer to data copy or create a new one
    CAICopy* pCopy = m_plCopyData->GetCopy( CAICopy::CBuildUnit );
    if ( pCopy == NULL )
    {
        try
        {
            pCopy = new CAICopy( CAICopy::CBuildUnit );
            m_plCopyData->AddTail( (CObject*)pCopy );
        }
        catch ( CException* e )
        {
            if ( pCopy != NULL )
            {
                delete pCopy;
                pCopy = NULL;
            }
            throw( ERR_CAI_BAD_NEW );
        }
    }

    // first clear out data slots
    for ( int i = 0; i < CAI_DATA_SLOTS; ++i )
    {
        pCopy->m_aiDataIn[i]  = 0;
        pCopy->m_aiDataOut[i] = 0;
    }

    // now update the current data copy from the game data
    for ( int i = 0; i < CMaterialTypes::num_build_types; ++i )
    {
        pCopy->m_aiDataIn[i] = pData->GetInput( i );
    }

    pCopy->m_aiDataIn[CAI_TYPEVEHICLE] = pData->GetVehType( );
    pCopy->m_aiDataOut[CAI_TIMEBUILD]  = pData->GetTime( );
}

void CAIUnit::Update( CStructureData const* pData )
{
    ASSERT_VALID( this );
    ASSERT_VALID( m_plCopyData );
    ASSERT_VALID( pData );

    // get current pointer to data copy or create a new one
    CAICopy* pCopy = m_plCopyData->GetCopy( CAICopy::CStructureData );
    if ( pCopy == NULL )
    {
        try
        {
            pCopy = new CAICopy( CAICopy::CStructureData );
            m_plCopyData->AddTail( (CObject*)pCopy );
        }
        catch ( CException* e )
        {
            if ( pCopy != NULL )
            {
                delete pCopy;
                pCopy = NULL;
            }
            throw( ERR_CAI_BAD_NEW );
        }
    }
    // first clear out data slots
    for ( int i = 0; i < CAI_DATA_SLOTS; ++i )
    {
        pCopy->m_aiDataIn[i]  = 0;
        pCopy->m_aiDataOut[i] = 0;
    }

    // now update the current data copy from the game data
    for ( int i = 0; i < CMaterialTypes::num_build_types; ++i )
    {
        pCopy->m_aiDataIn[i] = pData->GetBuild( i );
    }


    pCopy->m_aiDataOut[CAI_MORALE]    = pData->GetMoraleEffect( );
    pCopy->m_aiDataOut[CAI_POWER]     = pData->GetPower( );
    pCopy->m_aiDataOut[CAI_NOPOWER]   = pData->GetNoPower( );
    pCopy->m_aiDataOut[CAI_UNIONTYPE] = pData->GetUnionType( );

    pCopy->m_aiDataOut[CAI_TIMEBUILD] = pData->GetTimeBuild( );
}

void CAIUnit::Update( CTransportData const* /*pData*/ )
{
    ASSERT_VALID( this );
    ASSERT_VALID( m_plCopyData );
    // ASSERT_VALID( pData );

    // get current pointer to data copy or create a new one
    CAICopy* pCopy = m_plCopyData->GetCopy( CAICopy::CTransportData );
    if ( pCopy == NULL )
    {
        try
        {
            pCopy = new CAICopy( CAICopy::CTransportData );
            m_plCopyData->AddTail( (CObject*)pCopy );
        }
        catch ( CException* e )
        {
            if ( pCopy != NULL )
            {
                delete pCopy;
                pCopy = NULL;
            }
            throw( ERR_CAI_BAD_NEW );
        }
    }
    // now update the current data copy from the game data
    // BUGBUG CTransportData has no public access to its data
}


void CAIUnit::Update( CBuilding* pData )
{
    ASSERT_VALID( this );
    ASSERT_VALID( m_plCopyData );
    ASSERT_VALID( pData );

    // get current pointer to data copy or create a new one
    CAICopy* pCopy = m_plCopyData->GetCopy( CAICopy::CBuilding );
    if ( pCopy == NULL )
    {
        try
        {
            pCopy = new CAICopy( CAICopy::CBuilding );
            m_plCopyData->AddTail( (CObject*)pCopy );
        }
        catch ( CException* e )
        {
            if ( pCopy != NULL )
            {
                delete pCopy;
                pCopy = NULL;
            }
            throw( ERR_CAI_BAD_NEW );
        }
    }
    // first clear out data slots
    for ( int i = 0; i < CAI_DATA_SLOTS; ++i )
    {
        pCopy->m_aiDataIn[i]  = 0;
        pCopy->m_aiDataOut[i] = 0;
    }

    // now update the current data copy from the game data
    for ( int i = 0; i < CMaterialTypes::num_types; ++i )
    {
        pCopy->m_aiDataIn[i] = pData->GetStore( i );
    }

    CHexCoord hex;
    // hex = pData->GetHex();
    hex                           = pData->GetExitHex( );
    pCopy->m_aiDataOut[CAI_LOC_X] = hex.X( );
    pCopy->m_aiDataOut[CAI_LOC_Y] = hex.Y( );

    pCopy->m_aiDataOut[CAI_ISCONSTRUCTING] = (int)pData->IsConstructing( );
    pCopy->m_aiDataOut[CAI_ISPAUSED]       = (int)pData->IsPaused( );
    pCopy->m_aiDataOut[CAI_ISWAITING]      = (int)pData->IsWaiting( );
    pCopy->m_aiDataOut[CAI_PRODUCES]       = (int)pData->GetData( )->GetUnionType( );

    pCopy->m_aiDataOut[CAI_DAMAGE] = pData->GetDamagePer( );
    // pCopy->m_aiDataOut[CAI_EFFECTIVE] = pData->GetExperience();
    // pCopy->m_aiDataOut[CAI_AMMO] = pData->GetAmmo();
    //  the family type of the building type
    pCopy->m_aiDataOut[CAI_BUILDTYPE] = pData->GetData( )->GetBldgType( );
    // the specific type of the building
    pCopy->m_aiDataOut[CAI_TYPEBUILD] = pData->GetData( )->GetType( );
}

void CAIUnit::Update( CVehicle* pData )
{
    ASSERT_VALID( this );
    ASSERT_VALID( m_plCopyData );
    ASSERT_VALID( pData );

    // get current pointer to data copy or create a new one
    CAICopy* pCopy = m_plCopyData->GetCopy( CAICopy::CVehicle );
    if ( pCopy == NULL )
    {
        try
        {
            pCopy = new CAICopy( CAICopy::CVehicle );
            m_plCopyData->AddTail( (CObject*)pCopy );
        }
        catch ( CException* e )
        {
            if ( pCopy != NULL )
            {
                delete pCopy;
                pCopy = NULL;
            }
            throw( ERR_CAI_BAD_NEW );
        }
    }
    // first clear out data slots
    for ( int i = 0; i < CAI_DATA_SLOTS; ++i )
    {
        pCopy->m_aiDataIn[i]  = 0;
        pCopy->m_aiDataOut[i] = 0;
    }

    // now update the current data copy from the game data
    for ( int i = 0; i < CMaterialTypes::num_types; ++i )
    {
        pCopy->m_aiDataIn[i] = pData->GetStore( i );
    }


    CHexCoord hex                 = pData->GetHexHead( );
    pCopy->m_aiDataOut[CAI_LOC_X] = hex.X( );
    pCopy->m_aiDataOut[CAI_LOC_Y] = hex.Y( );

    // BUGBUG no public access to prev and dest hexes
    // pCopy->m_aiDataOut[CAI_PREV_X] = pData->m_hexPrev.m_iX;
    // pCopy->m_aiDataOut[CAI_PREV_Y] = pData->m_hexPrev.m_iY;
    CHexCoord hexDest              = pData->GetHexDest( );
    pCopy->m_aiDataOut[CAI_DEST_X] = hexDest.X( );
    pCopy->m_aiDataOut[CAI_DEST_Y] = hexDest.Y( );

    // pCopy->m_aiDataOut[CAI_FUEL] = pData->GetFuel();
    pCopy->m_aiDataOut[CAI_DAMAGE] = pData->GetDamagePer( );
    // pCopy->m_aiDataOut[CAI_EFFECTIVE] = pData->GetExperience();
    // pCopy->m_aiDataOut[CAI_AMMO] = pData->GetAmmo();
    //  the specific type of the vehicle
    pCopy->m_aiDataOut[CAI_TYPEVEHICLE] = pData->GetData( )->GetType( );
}
//
// get a current pointer to data copy based on
// the type of data requested
//
CAICopy* CAIUnit::GetCopyData( int iType )
{
    ASSERT_VALID( this );
    ASSERT_VALID( m_plCopyData );

    CVehicle*  pVehicle = NULL;
    CBuilding* pBldg    = NULL;

    EnterCriticalSection( &cs );

    switch ( iType )
    {
    case CAICopy::CUnitData:
        if ( m_iType == CUnit::building )
        {
            CBuilding const* pBldg = pGameData->GetBuildingData( m_iOwner, m_dwID );
            if ( pBldg == NULL )
                break;
            CStructureData const* pData = pBldg->GetData( );
            Update( (CUnitData const*)pData );
        }
        else if ( m_iType == CUnit::vehicle )
        {
            CVehicle* pVehicle = pGameData->GetVehicleData( m_iOwner, m_dwID );
            if ( pVehicle == NULL )
                break;

            CTransportData const* pData = pVehicle->GetData( );
            Update( (CUnitData const*)pData );
        }
        break;

    case CAICopy::CVehicle:
    case CAICopy::CTransportData:

        pVehicle = pGameData->GetVehicleData( m_iOwner, m_dwID );
        if ( pVehicle == NULL )
            break;

        // update CVehicle copy data
        if ( iType == CAICopy::CVehicle )
            Update( pVehicle );
        // update CTransportData copy data
        else if ( iType == CAICopy::CTransportData )
            Update( pVehicle->GetData( ) );

        break;

    case CAICopy::CBuilding:
    case CAICopy::CStructureData:
    case CAICopy::CBuildMaterials:
    case CAICopy::CBuildMine:
    case CAICopy::CBuildFarm:
    // BUGBUG build units are now an array of CBuildUnit objects
    // contained in the CBuildVehicle pointer from CStructureData
    //
    case CAICopy::CBuildUnit:

        // get CBuilding *pBldg from game for this unit
        pBldg = pGameData->GetBuildingData( m_iOwner, m_dwID );
        if ( pBldg == NULL )
            break;

        // update CBuilding copy data
        if ( iType == CAICopy::CBuilding )
            Update( pBldg );
        // update CStructureData copy data
        else if ( iType == CAICopy::CStructureData )
            Update( pBldg->GetData( ) );
        // update CBuildMaterials copy data
        else if ( iType == CAICopy::CBuildMaterials )
            Update( pBldg->GetData( )->GetBldMaterials( ) );
        // update CBuildMine copy data
        else if ( iType == CAICopy::CBuildMine )
            Update( pBldg->GetData( )->GetBldMine( ) );
        // update CBuildFarm copy data
        else if ( iType == CAICopy::CBuildFarm )
            Update( pBldg->GetData( )->GetBldFarm( ) );
        // update CBuildUnit copy data
        else if ( iType == CAICopy::CBuildUnit )
        {
            if ( pBldg->GetData( )->GetUnionType( ) == CStructureData::UTvehicle )
            {
                // capture data based on what the building
                // is producing or the 1st thing it could
                // produce if it is not producing anything
                CVehicleBuilding* pVehBldg  = (CVehicleBuilding*)pBldg;
                CBuildUnit const* pBuildVeh = pVehBldg->GetBldUnt( );

                if ( pBuildVeh != NULL )
                    Update( pBuildVeh );

                // if the building is not currently producing
                // a vehicle, then allow a NULL return
            }
            if ( pBldg->GetData( )->GetUnionType( ) == CStructureData::UTrepair )
            {
                // convert current building to a repair building
                CRepairBuilding* pRepBldg = (CRepairBuilding*)pBldg;
                // is there a vehicle in getting a repair?
                CVehicle* pVehRepairing = pRepBldg->GetVehRepairing( );
                if ( pVehRepairing == NULL )
                    break;
                // get the repair data for that vehicle
                CBuildRepair const* pBuildRep = pRepBldg->GetData( );

                // BUGBUG may get total materials needed from using
                // pBuildRep->GetTotalRepair(iInd)

                CBuildUnit const* pBuildVeh = pBuildRep->GetRepair( pVehRepairing->GetData( )->GetType( ) );
                if ( pBuildVeh != NULL )
                    Update( pBuildVeh );
            }
        }
        break;
    }
    LeaveCriticalSection( &cs );

    return ( m_plCopyData->GetCopy( iType ) );
}

/*
// Vehicle must be in repair building when this is sent
class CMsgRepairVeh : public CNetCmd
{
public:
        CMsgRepairVeh () : CNetCmd (repair_veh) {}
        CMsgRepairVeh (CVehicle const * pVeh);

    DWORD			m_dwIDVeh;
};

*/
void CAIUnit::RepairVehicle( void )
{
    CMsgRepairVeh msg;
    msg.m_dwIDVeh = m_dwID;

    theGame.PostToServer( (CNetCmd*)&msg, sizeof( CMsgRepairVeh ) );
    // theGame.PostToAll( (CNetCmd *)&msg, sizeof(CMsgRepairVeh) );
}


/*
// Vehicle must be in repair building when this is sent
class CMsgRepairBldg : public CNetCmd
{
public:
        CMsgRepairBldg () : CNetCmd (repair_bldg) {}
        CMsgRepairBldg (CVehicle const * pVeh, CBuilding const * pBldg);

    DWORD			m_dwIDVeh;
    DWORD			m_dwIDBldg;
};
*/
//
// causes a crane to repair a damaged building it is in
//
void CAIUnit::RepairBuilding( void )
{
    CMsgRepairBldg msg;
    msg.m_dwIDVeh  = m_dwID;
    msg.m_dwIDBldg = m_dwData;

    theGame.PostToServer( (CNetCmd*)&msg, sizeof( CMsgRepairBldg ) );
    // theGame.PostToAll( (CNetCmd *)&msg, sizeof(CMsgRepairBldg) );
}

/* // as of 2/18/96 this is the message
// unload a carrier
class CMsgUnloadCarrier : public CNetCmd
{
public:
        CMsgUnloadCarrier () : CNetCmd (unload_carrier) {}
        CMsgUnloadCarrier (CVehicle const * pVeh);

    DWORD		m_dwID;		// unit carrying it
};
*/

//
// issue the message to unload the carrier unit
//
void CAIUnit::UnloadCargo( void )
{
    CMsgUnloadCarrier msg;
    msg.m_dwID = m_dwID;

    // theGame.PostToServer( (CNetCmd *)&msg, sizeof(CMsgUnloadCarrier) );
    theGame.PostToAll( (CNetCmd*)&msg, sizeof( CMsgUnloadCarrier ) );
}

//
// issue the message to load the cargo unit
//
void CAIUnit::LoadUnit( DWORD dwCargo )
{
    /* // load a carrier
    class CMsgLoadCarrier : public CNetCmd
    {
    public:
            CMsgLoadCarrier () : CNetCmd (load_carrier) {}
            CMsgLoadCarrier (CVehicle const * pCargo, CVehicle const * pCarrier);

        DWORD		m_dwIDCargo;			// unit to carry
        DWORD		m_dwIDCarrier;		// unit carrying it
    };
    */
    CMsgLoadCarrier msg;
    msg.m_dwIDCargo   = dwCargo;
    msg.m_dwIDCarrier = m_dwID;
    // theGame.PostToServer( (CNetCmd *)&msg, sizeof(CMsgLoadCarrier) );
    theGame.PostToAll( (CNetCmd*)&msg, sizeof( CMsgLoadCarrier ) );
}

//
// set the destination of this unit to that
// of the passed building
//
void CAIUnit::SetDestination( CAIUnit* pCAIBldg )
{
    ASSERT_VALID( this );

    EnterCriticalSection( &cs );

    // BUGBUG get current location of the unit for which
    // the destination is being set to make sure the unit
    // is not being requested to go to its current location
    CHexCoord hexVeh;
    CVehicle* pVehicle = pGameData->GetVehicleData( m_iOwner, m_dwID );
    if ( pVehicle != NULL )
        hexVeh = pVehicle->GetHexHead( );

    CBuilding* pBldg = pGameData->GetBuildingData( pCAIBldg->GetOwner( ), pCAIBldg->GetID( ) );
    if ( pBldg != NULL )
    {
        // this should be the entrance to the building?
        // CHexCoord hex = pBldg->GetHex();
        CHexCoord hex = pBldg->GetExitHex( );

        // CMsgVehSetDest (DWORD dwID, CHexCoord const & hex, int iMode);
        // pMsg = new CMsgVehSetDest( m_dwID, hex, CVehicle::_goto );
        CMsgVehSetDest msg( m_dwID, hex, CVehicle::moving );
        LeaveCriticalSection( &cs );

        // don't bother if current location is the same as destination
        if ( hex == hexVeh )
            return;

#ifdef _LOGOUT

        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIUnit::SetDestination() player %d unit %ld going to %d,%d \n",
                   m_iOwner, m_dwID, hex.X( ), hex.Y( ) );
#endif
        theGame.PostToServer( (CNetCmd*)&msg, sizeof( CMsgVehSetDest ) );
        return;
    }

    LeaveCriticalSection( &cs );
}
//
// send a vehicle to a specific sub-hex, possibly to load
// but maybe not
//
void CAIUnit::SetDestination( CSubHex& subHexDest )
{
    ASSERT_VALID( this );

    BOOL bNobug = FALSE;
    if ( subHexDest.x > 512 || subHexDest.y > 512 )
        bNobug = TRUE;

    // BUGBUG get current location of the unit for which
    // the destination is being set to make sure the unit
    // is not being requested to go to its current location
    EnterCriticalSection( &cs );
    CSubHex   subHexVeh;
    CVehicle* pVehicle = pGameData->GetVehicleData( m_iOwner, m_dwID );
    if ( pVehicle != NULL )
        subHexVeh = pVehicle->GetPtHead( );
    LeaveCriticalSection( &cs );

    // don't bother if current location is the same as destination
    if ( subHexDest == subHexVeh )
        return;

    // CMsgVehSetDest (DWORD dwID, CSubHex const & hex, int iMode);
    /*
    CMsgVehSetDest::CMsgVehSetDest (DWORD dwID, CHexCoord const & hex, int iMode) : CNetCmd (veh_set_dest)
    {

        m_dwID = dwID;
        m_hex = hex;
        m_sub = CSubHex (hex.X()*2, hex.Y()*2);
        m_iSub = CVehicle::full;
        m_iMode = iMode;
        ASSERT_CMD (this);
    }
    */

    // kludge because the subhex form of the ctor is not implemented
    CHexCoord      hex( ( subHexDest.x / 2 ), ( subHexDest.y / 2 ) );
    CMsgVehSetDest msg( m_dwID, hex, CVehicle::moving );
    msg.m_sub = subHexDest;

    // CMsgVehSetDest msg( m_dwID, subHexDest, CVehicle::moving );
    theGame.PostToServer( (CNetCmd*)&msg, sizeof( CMsgVehSetDest ) );
}

//
// set the destination of this unit to the passed hex
//
void CAIUnit::SetDestination( CHexCoord& m_hex )
{
    ASSERT_VALID( this );

    // if we gave it this dest in the last minute - don't repeat
    if ( m_hex == m_hexLastDest )
        if ( theGame.GettimeGetTime( ) < m_timeLastDest + 30 * 1000 )
            return;
    m_hexLastDest  = m_hex;
    m_timeLastDest = theGame.GettimeGetTime( );

    // BUGBUG get current location of the unit for which
    // the destination is being set to make sure the unit
    // is not being requested to go to its current location
    EnterCriticalSection( &cs );
    CHexCoord hexVeh;
    CVehicle* pVehicle = pGameData->GetVehicleData( m_iOwner, m_dwID );
    if ( pVehicle != NULL )
        hexVeh = pVehicle->GetHexHead( );
    LeaveCriticalSection( &cs );

    // don't bother if current location is the same as destination
    if ( m_hex == hexVeh )
        return;

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIUnit::SetDestination() player %d unit %ld going to %d,%d \n",
               m_iOwner, m_dwID, m_hex.X( ), m_hex.Y( ) );
#endif

    // CMsgVehSetDest (DWORD dwID, CHexCoord const & hex, int iMode);
    CMsgVehSetDest msg( m_dwID, m_hex, CVehicle::moving );
    theGame.PostToServer( (CNetCmd*)&msg, sizeof( CMsgVehSetDest ) );
}

/*
// sent by AI to attack a vehicle
class CMsgAttack : public CNetCmd
{
public:
        CMsgAttack (DWORD dwShooter, DWORD dwTarget);

        DWORD				m_dwShooter;		// unit shooting
        DWORD				m_dwTarget;			// unit being shot at

#ifdef _DEBUG
public:
    void AssertValid() const;
#endif
};
*/

//
// calls the game object attack function?
//
// the actual message, CMsgAttackUnit is not available (as of 12/28/95)
// which will allow the AI to attack a unit.  when available it will
// probably be dwID driven and not hex as indicated below
//
// void CAIUnit::AttackHex( CHexCoord& m_hex )
void CAIUnit::AttackUnit( DWORD dwTarget )
{
    ASSERT_VALID( this );

    // also, need to record the dwID of this CAIUnit
    // as attacking the targeted opfor unit by putting
    // it in the dwParams of the target unit and recording
    // the targeted opfor in the CAIUnit::m_dwData
    m_dwData = dwTarget;

    CMsgAttack msg( m_dwID, dwTarget );
    theGame.PostToServer( (CNetCmd*)&msg, sizeof( CMsgAttack ) );
}

//
// return the number of units attacking this unit
//
int CAIUnit::AttackCount( void )
{
    int iCnt = 0;
    for ( int i = 0; i < CAI_SIZE; ++i )
    {
        if ( m_pdwaParams[i] )
            iCnt++;
    }
    return ( iCnt );
}

//
// OPFOR units that have been created, have the dwIDs of the
// CAIMgr units that are attacking them, stored in the m_pdwaParams[]
//
void CAIUnit::AttackedBy( DWORD dwAttacker )
{
    for ( int i = 0; i < CAI_SIZE; ++i )
    {
        // only store it once
        if ( m_pdwaParams[i] == dwAttacker )
            return;
        if ( !m_pdwaParams[i] )
        {
            m_pdwaParams[i] = dwAttacker;
            return;
        }
    }
}


//
// OPFOR units that have been created, have the dwIDs of the
// CAIMgr units that are attacking them, stored in the m_pdwaParams[]
// so any non-zero entry indicates some CAIMgr unit is attacking
// this OPFOR unit
//
// AI PLAYER units use m_pdwaParams[] for other things
//
BOOL CAIUnit::IsAttacked( void )
{
    for ( int i = 0; i < CAI_SIZE; ++i )
    {
        if ( m_pdwaParams[i] )
            return TRUE;
    }
    return FALSE;
}

/*
// tell 2 units to transfer materials
class CMsgTransMat : public CNetCmd
{
public:
        CMsgTransMat (CUnit const *pSrc, CUnit const *pDest);

        DWORD				m_dwIDSrc;		// ID of unit giving up materials
        DWORD				m_dwIDDest;		// ID of unit receiving materials
        short int		m_aiMat [CMaterialTypes::num_types];	// materials to transfer

#ifdef _DEBUG
public:
    void AssertValid() const;
#endif
};
*/
//
// move the material ided by the iMaterial, from the unit ided
// by dwFromID to the unit ided by dwToID
//
void CAIUnit::MoveMaterial( int iMaterial, int iQty, DWORD dwFromID, DWORD dwToID )
{
    CMsgTransMat msg;
    msg.m_dwIDSrc  = dwFromID;
    msg.m_dwIDDest = dwToID;
    memset( msg.m_aiMat, 0, sizeof( msg.m_aiMat ) );
    msg.m_aiMat[iMaterial] = iQty;
    theGame.PostToServer( (CNetCmd*)&msg, sizeof( CMsgTransMat ) );
}

WORD CAIUnit::GetParam( WORD wOffset )
{
    ASSERT_VALID( this );
    if ( m_pwaParams == NULL )
        return ( FALSE );
    ASSERT( wOffset < CAI_SIZE );
    if ( wOffset < CAI_SIZE )
        return ( m_pwaParams[wOffset] );
    return ( FALSE );
}

void CAIUnit::SetParam( WORD wOffset, WORD wValue )
{
    ASSERT_VALID( this );
    if ( m_pwaParams == NULL )
        return;
    ASSERT( wOffset < CAI_SIZE );
    if ( wOffset < CAI_SIZE )
        m_pwaParams[wOffset] = wValue;
}

DWORD CAIUnit::GetParamDW( WORD wOffset )
{
    ASSERT_VALID( this );
    if ( m_pdwaParams == NULL )
        return ( FALSE );

    ASSERT( wOffset < CAI_SIZE );
    if ( wOffset < CAI_SIZE )
        return ( m_pdwaParams[wOffset] );
    return ( FALSE );
}

void CAIUnit::SetParamDW( WORD wOffset, DWORD dwValue )
{
    ASSERT_VALID( this );
    if ( m_pdwaParams == NULL )
        return;

    ASSERT( wOffset < CAI_SIZE );
    if ( wOffset < CAI_SIZE )
        m_pdwaParams[wOffset] = dwValue;
}

//
// clears only DW parameter array
//
void CAIUnit::ClearParamDW( void )
{
    ASSERT_VALID( this );
    if ( m_pdwaParams == NULL )
        return;
    for ( int i = 0; i < CAI_SIZE; ++i )
    {
        m_pdwaParams[i] = 0;
    }
}
//
// clears both parameter arrays
//
void CAIUnit::ClearParam( void )
{
    ASSERT_VALID( this );
    if ( m_pwaParams == NULL || m_pdwaParams == NULL )
        return;
    for ( int i = 0; i < CAI_SIZE; ++i )
    {
        m_pwaParams[i]  = 0;
        m_pdwaParams[i] = 0;
    }
}

DWORD CAIUnit::GetID( void )
{
    ASSERT_VALID( this );
    return m_dwID;
}

void CAIUnit::SetID( DWORD dwID )
{
    ASSERT_VALID( this );
    m_dwID = dwID;
}

int CAIUnit::GetOwner( void )
{
    ASSERT_VALID( this );
    return m_iOwner;
}

void CAIUnit::SetOwner( int iOwner )
{
    ASSERT_VALID( this );
    m_iOwner = iOwner;
}

int CAIUnit::GetType( void )
{
    ASSERT_VALID( this );
    return m_iType;
}

void CAIUnit::SetType( int iType )
{
    ASSERT_VALID( this );
    m_iType = iType;
}

int CAIUnit::GetTypeUnit( void )
{
    ASSERT_VALID( this );
    return m_iTypeUnit;
}


BOOL CAIUnit::IsControl( void )
{
    return m_bControl;
}

void CAIUnit::SetControl( BOOL bControl )
{
    m_bControl = bControl;
}

WORD CAIUnit::GetGoal( void )
{
    ASSERT_VALID( this );
    return m_wGoal;
}

void CAIUnit::SetGoal( WORD wGoal )
{
    ASSERT_VALID( this );
    m_wGoal = wGoal;
}

WORD CAIUnit::GetTask( void )
{
    ASSERT_VALID( this );
    return m_wTask;
}

void CAIUnit::SetTask( WORD wTask )
{
    ASSERT_VALID( this );
    m_wTask = wTask;
}

DWORD CAIUnit::GetDataDW( void )
{
    ASSERT_VALID( this );
    return m_dwData;
}

void CAIUnit::SetDataDW( DWORD dwData )
{
    ASSERT_VALID( this );
    m_dwData = dwData;
}

WORD CAIUnit::GetStatus( void )
{
    ASSERT_VALID( this );
    return m_wStatus;
}

void CAIUnit::SetStatus( WORD wStatus )
{
    ASSERT_VALID( this );
    m_wStatus = wStatus;
}

CAIUnit::~CAIUnit( )
{
    ASSERT_VALID( this );

    if ( m_plCopyData != NULL )
    {
        delete m_plCopyData;
        m_plCopyData = NULL;
    }

    if ( m_pwaParams != NULL )
    {
        delete[] m_pwaParams;
        m_pwaParams = NULL;
    }
    if ( m_pdwaParams != NULL )
    {
        delete[] m_pdwaParams;
        m_pdwaParams = NULL;
    }
}


/////////////////////////////////////////////////////////////////////////////


CAIUnitList::~CAIUnitList( void )
{
    ASSERT_VALID( this );
    DeleteList( );
}
//
// retrieve a pointer to a unit controlled by another player
// either HP or AI.
//
CAIUnit* CAIUnitList::GetOpForUnit( DWORD dwID )
{
    // if already known, then return it
    CAIUnit* pOpFor = GetUnit( dwID );
    if ( pOpFor != NULL )
        return ( pOpFor );

    // else get a copy from the game and save it

    // NOTE: the thread must be granted exclusive access
    EnterCriticalSection( &cs );

    CVehicle* pVehicle = theVehicleMap.GetVehicle( dwID );
    if ( pVehicle != NULL )
    {
        if ( !pVehicle->IsFlag( CUnit::dying ) )
        {
            try
            {
                // CAIUnit::CAIUnit( DWORD dwID, int iOwner, int iType )
                pOpFor = new CAIUnit( dwID, pVehicle->GetOwner( )->GetPlyrNum( ), pVehicle->GetUnitType( ),
                                      pVehicle->GetData( )->GetType( ) );

                AddTail( (CObject*)pOpFor );
            }
            catch ( CException* e )
            {
                LeaveCriticalSection( &cs );
                throw( ERR_CAI_BAD_NEW );
            }
        }
    }
    else
    {
        CBuilding* pBldg = theBuildingMap.GetBldg( dwID );
        if ( pBldg != NULL )
        {
            if ( !pBldg->IsFlag( CUnit::dying ) )
            {
                try
                {
                    // CAIUnit::CAIUnit( DWORD dwID, int iOwner, int iType )
                    pOpFor = new CAIUnit( dwID, pBldg->GetOwner( )->GetPlyrNum( ), pBldg->GetUnitType( ),
                                          pBldg->GetData( )->GetType( ) );

                    AddTail( (CObject*)pOpFor );
                }
                catch ( CException* e )
                {
                    LeaveCriticalSection( &cs );
                    throw( ERR_CAI_BAD_NEW );
                }
            }
        }
    }
    LeaveCriticalSection( &cs );

    return ( pOpFor );
}

//
// find the closest repair type of building passed, that has the fewest
// units assigned for repair, that is the closest to the hex passed
//
CAIUnit* CAIUnitList::GetClosestRepair( int iPlayer, int iBldg, CHexCoord& hex )
{
    ASSERT_VALID( this );
    CHexCoord hexBldg, hexBest;
    int       iBest = 0xFFFE, iDist, iAssigned, iRepairCount;
    iRepairCount    = iBest;

    CAIUnit* paiBldg = NULL;
    BOOL     bBoat   = FALSE;
    if ( iBldg != CStructureData::repair )
        bBoat = TRUE;


    POSITION pos = GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIUnit* pRepBldg = (CAIUnit*)GetNext( pos );
        if ( pRepBldg != NULL )
        {
            ASSERT_VALID( pRepBldg );

            if ( pRepBldg->GetOwner( ) != iPlayer )
                continue;

            // since there are 2 types of ship repair centers and one
            // type of vehicle repair center, we have to hard code to tell
            if ( bBoat )
            {
                if ( pRepBldg->GetTypeUnit( ) != CStructureData::shipyard_1 &&
                     pRepBldg->GetTypeUnit( ) != CStructureData::shipyard_3 )
                    continue;
            }
            else  // iBldg should be CStructureData::repair
            {
                if ( pRepBldg->GetTypeUnit( ) != iBldg )
                    continue;
            }

            BOOL bIsConstructing = FALSE;
            hexBldg.X( 0 );
            hexBldg.Y( 0 );
            EnterCriticalSection( &cs );
            CBuilding* pBldg = theBuildingMap.GetBldg( pRepBldg->GetID( ) );
            if ( pBldg != NULL )
            {
                hexBldg         = pBldg->GetExitHex( );
                bIsConstructing = pBldg->IsConstructing( );
            }
            LeaveCriticalSection( &cs );

            if ( bIsConstructing )
                continue;
            if ( !hexBldg.X( ) && !hexBldg.Y( ) )
                continue;

#if THREADS_ENABLED
            // BUGBUG this function must yield
            myYieldThread( );
            // if( myYieldThread() == TM_QUIT )
            //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
            // consider that units of the appropriate type may be already
            // assigned for repair at that center, and therefore we want
            // the repair center with the fewest units assigned that is
            // the closest to the hex passed
            iAssigned = GetRepairCount( iPlayer, pRepBldg->GetID( ) );
            if ( iAssigned < iRepairCount )
            {
                iRepairCount = iAssigned;
                iBest        = 0xFFFE;
            }

            // skip repair centers with more than detected fewest assignments
            if ( iAssigned > iRepairCount )
                continue;

            // repair centers with the same number assigned, as fewest,
            // are allowed to continue for range test

            // consider range to desired hex, looking for closest
            iDist = pGameData->GetRangeDistance( hex, hexBldg );
            if ( iDist && iDist < iBest )
            {
                paiBldg = pRepBldg;
                iBest   = iDist;
                hexBest = hexBldg;
            }
        }
    }

    if ( paiBldg != NULL )
    {
        hex = hexBest;
        return ( paiBldg );
    }
    return ( NULL );
}

//
// count number of vehicle units which have the passed dwID in
// their dwData, indicating this is unit assigned for repair
// at that repair center
//
int CAIUnitList::GetRepairCount( int iPlayer, DWORD dwRepBldg )
{
    int iCount = 0;

    POSITION pos = GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIUnit* pUnit = (CAIUnit*)GetNext( pos );
        if ( pUnit != NULL )
        {
#if AI_THREADS_ENABLED
       //  BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
            ASSERT_VALID( pUnit );

            if ( pUnit->GetOwner( ) != iPlayer )
                continue;
            if ( pUnit->GetType( ) != CUnit::vehicle )
                continue;
            // this vehicle is assigned to be repaired at the
            // repair center passed in
            if ( pUnit->GetDataDW( ) == dwRepBldg )
                iCount++;
        }
    }
    return ( iCount );
}

//
// find the building of the type passed, belonging to the player passed,
// that is closest to the hex passed
//
CAIUnit* CAIUnitList::GetClosest( int iPlayer, int iBldg, CHexCoord& hex )
{
    ASSERT_VALID( this );
    CHexCoord hexBldg, hexBest;
    int       iBest   = 0xFFFE, iDist;
    CAIUnit*  paiBldg = NULL;

    POSITION pos = GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIUnit* pUnit = (CAIUnit*)GetNext( pos );
        if ( pUnit != NULL )
        {
#if THREADS_ENABLED
            // BUGBUG this function must yield
            myYieldThread( );
            // if( myYieldThread() == TM_QUIT )
            //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
            ASSERT_VALID( pUnit );

            if ( pUnit->GetOwner( ) != iPlayer )
                continue;
            if ( pUnit->GetTypeUnit( ) != iBldg )
                continue;

            hexBldg.X( 0 );
            hexBldg.Y( 0 );
            EnterCriticalSection( &cs );
            CBuilding* pBldg = theBuildingMap.GetBldg( pUnit->GetID( ) );
            if ( pBldg != NULL )
            {
                hexBldg = pBldg->GetExitHex( );
            }
            LeaveCriticalSection( &cs );

            if ( !hexBldg.X( ) && !hexBldg.Y( ) )
                continue;

            iDist = pGameData->GetRangeDistance( hex, hexBldg );
            if ( iDist && iDist < iBest )
            {
                paiBldg = pUnit;
                iBest   = iDist;
                hexBest = hexBldg;
            }
        }
    }

    if ( paiBldg != NULL )
    {
        hex = hexBest;
        return ( paiBldg );
    }
    return ( NULL );
}

//
// return a pointer to the CAIUnit object matching the id passed
//
CAIUnit* CAIUnitList::GetUnitNY( DWORD dwId )
{
    ASSERT_VALID( this );

    POSITION pos = GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIUnit* pUnit = (CAIUnit*)GetNext( pos );
        if ( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

            if ( pUnit->GetID( ) == dwId )
                return ( pUnit );
        }
    }
    return ( NULL );
}
//
// return a pointer to the CAIUnit object matching the id passed
//
CAIUnit* CAIUnitList::GetUnit( DWORD dwId )
{
    ASSERT_VALID( this );

    POSITION pos = GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIUnit* pUnit = (CAIUnit*)GetNext( pos );
        if ( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

            if ( pUnit->GetID( ) == dwId )
                return ( pUnit );
        }
    }
    return ( NULL );
}

//
// clears the units that are targeting the unit passed
//
void CAIUnitList::ClearTarget( DWORD dwTarget )
{
    ASSERT_VALID( this );

    POSITION pos = GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIUnit* pUnit = (CAIUnit*)GetNext( pos );
        if ( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

            if ( pUnit->GetDataDW( ) == dwTarget )
                pUnit->SetDataDW( 0 );
        }
    }
}

//
// delete CAIUnit objects in the list
//
void CAIUnitList::DeleteList( void )
{
    ASSERT_VALID( this );

    if ( GetCount( ) )
    {
        POSITION pos = GetHeadPosition( );
        while ( pos != NULL )
        {
            CAIUnit* pUnit = (CAIUnit*)GetNext( pos );
            if ( pUnit != NULL )
            {
                ASSERT_VALID( pUnit );

                delete pUnit;
            }
        }
    }
    RemoveAll( );
}

void CAIUnitList::AddUnit( DWORD dwID )
{
    int  iPlayer   = 0;
    int  iType     = 0xFFFE;
    int  iUnitType = 0xFFFE;
    BOOL bControl  = FALSE;

    EnterCriticalSection( &cs );
    CVehicle* pVehicle = theVehicleMap.GetVehicle( dwID );
    if ( pVehicle != NULL )
    {
        iPlayer   = pVehicle->GetOwner( )->GetPlyrNum( );
        iType     = CUnit::vehicle;
        iUnitType = pVehicle->GetData( )->GetType( );
        bControl  = pVehicle->GetOwner( )->IsAI( );
    }
    else
    {
        CBuilding* pBldg = theBuildingMap.GetBldg( dwID );
        if ( pBldg != NULL )
        {
            iPlayer   = pBldg->GetOwner( )->GetPlyrNum( );
            iType     = CUnit::building;
            iUnitType = pBldg->GetData( )->GetType( );
            bControl  = pBldg->GetOwner( )->IsAI( );
        }
        else
            return;
    }
    LeaveCriticalSection( &cs );

    if ( !iPlayer )
        return;

    CAIUnit* pUnit = NULL;
    try
    {
        // CAIUnit( DWORD dwID, int iOwner, class, type );
        pUnit = new CAIUnit( dwID, iPlayer, iType, iUnitType );
        ASSERT_VALID( pUnit );
        AddTail( (CObject*)pUnit );
    }
    catch ( CException* e )
    {
        // need to report this error occurred
        throw( ERR_CAI_BAD_NEW );
    }

    if ( pUnit == NULL )
        return;

    // indicate what controls this unit
    pUnit->SetControl( bControl );
}

//
// remove all units belonging to this player
//
void CAIUnitList::RemoveUnits( int iPlayer )
{
    ASSERT_VALID( this );

    CAIUnit* pUnit = NULL;
    POSITION pos1, pos2;
    for ( pos1 = GetHeadPosition( ); ( pos2 = pos1 ) != NULL; )
    {
        pUnit = (CAIUnit*)GetNext( pos1 );
        if ( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

            if ( pUnit->GetOwner( ) != iPlayer )
                continue;
        }

        pUnit = (CAIUnit*)GetAt( pos2 );
        if ( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

            RemoveAt( pos2 );
            delete pUnit;
        }
    }
}

//
// remove the single unit indicated by the dwID (with optional flag
// to delete the actual object too)
//
void CAIUnitList::RemoveUnit( DWORD dwID, BOOL bObjectToo /*=TRUE*/ )
{
    ASSERT_VALID( this );

    // no need to remove that which is not there
    CAIUnit* pUnit = GetUnit( dwID );
    if ( pUnit == NULL )
        return;

    POSITION pos1, pos2;
    for ( pos1 = GetHeadPosition( ); ( pos2 = pos1 ) != NULL; )
    {
        pUnit = (CAIUnit*)GetNext( pos1 );
        if ( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

            if ( pUnit->GetID( ) != dwID )
                continue;
        }

        pUnit = (CAIUnit*)GetAt( pos2 );
        if ( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

            RemoveAt( pos2 );
            if ( bObjectToo )
                delete pUnit;
            break;
        }
    }
}


void CAIUnitList::Load( CFile* pFile )
{
    // make any old units go away
    if ( GetCount( ) )
        DeleteList( );

    UnitBuff ubUnit;

    // now get count of units
    int iCnt;
    pFile->Read( (void*)&iCnt, sizeof( int ) );

    if ( iCnt )
    {
        int iBytes;
        for ( int i = 0; i < iCnt; ++i )
        {
            // read file data into buffer
            iBytes = pFile->Read( (void*)&ubUnit, sizeof( UnitBuff ) );

            // BUGBUG how should read errors be reported?
            if ( iBytes != sizeof( UnitBuff ) )
                return;

            // CAIUnit( DWORD dwID, int iOwner, int iType, int iTypeUnit );
            CAIUnit* pUnit = new CAIUnit( ubUnit.dwID, ubUnit.iOwner, ubUnit.iType, ubUnit.iTypeUnit );

            pUnit->SetControl( ubUnit.bControl );
            pUnit->SetGoal( ubUnit.wGoal );
            pUnit->SetTask( ubUnit.wTask );
            pUnit->SetDataDW( ubUnit.dwData );
            pUnit->SetStatus( ubUnit.wStatus );

            for ( int i = 0; i < CAI_SIZE; ++i )
            {
                pUnit->SetParam( i, ubUnit.iParams[i] );
                pUnit->SetParamDW( i, ubUnit.dwParams[i] );
            }

            // put the unit in the list
            AddTail( (CObject*)pUnit );
        }
    }
}

void CAIUnitList::Save( CFile* pFile )
{
    UnitBuff ubUnit;

    int iCnt = GetCount( );
    pFile->Write( (const void*)&iCnt, sizeof( int ) );

    if ( iCnt )
    {
        POSITION pos = GetHeadPosition( );
        while ( pos != NULL )
        {
            CAIUnit* pUnit = (CAIUnit*)GetNext( pos );
            if ( pUnit != NULL )
            {
                ubUnit.dwID      = pUnit->GetID( );
                ubUnit.iOwner    = pUnit->GetOwner( );
                ubUnit.iType     = pUnit->GetType( );
                ubUnit.iTypeUnit = pUnit->GetTypeUnit( );
                ubUnit.bControl  = pUnit->IsControl( );
                ubUnit.wGoal     = pUnit->GetGoal( );
                ubUnit.wTask     = pUnit->GetTask( );
                ubUnit.dwData    = pUnit->GetDataDW( );
                ubUnit.wStatus   = pUnit->GetStatus( );

                for ( int i = 0; i < CAI_SIZE; ++i )
                {
                    ubUnit.iParams[i]  = pUnit->GetParam( i );
                    ubUnit.dwParams[i] = pUnit->GetParamDW( i );
                }

                pFile->Write( (const void*)&ubUnit, sizeof( UnitBuff ) );
            }
        }
    }
}

// end of CAIUnit.cpp
