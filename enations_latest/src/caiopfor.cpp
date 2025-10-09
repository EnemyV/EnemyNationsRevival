////////////////////////////////////////////////////////////////////////////
//
//  CAIOpFor.cpp : CAIOpFor object implementation
//                 Divide and Conquer AI
//
//  Last update:    09/08/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "caiopfor.hpp"

#include "cai.h"
#include "caidata.hpp"
#include "stdafx.h"

#define new DEBUG_NEW

extern CException*      pException;  // standard exception for yielding
extern CRITICAL_SECTION cs;          // used by threads
extern CAIData*         pGameData;   // pointer to game data interface

/////////////////////////////////////////////////////////////////////////////

CAIOpFor::CAIOpFor( int iPlayer, const char* pzName )
{
    m_iPlayer          = iPlayer;
    m_sName            = pzName;
    m_pwaUnits         = NULL;
    m_pwaBldgs         = NULL;
    m_pwaAttackedUnits = NULL;
    m_pwaAttackedBldgs = NULL;
    m_pwaRAs           = NULL;
    m_dwRocket         = 0;
    m_iMsgCount        = 0;
    m_cRelations       = NEUTRAL;
    m_iAttitude        = (int)m_cRelations;

    m_bAtWar = FALSE;
    // allow difficulty to influence opfors to war status
    // at the start of the game
    // if( pGameData->m_iSmart >= (NUM_DIFFICUTY_LEVELS/2) )
    //	m_bAtWar = TRUE;
    //
    // BUGBUG - force at war for CGDC demo, for all levels but easy
    //
    // if( pGameData->m_iSmart )
    //	m_bAtWar = TRUE;

    m_bIsAI  = FALSE;
    m_bKnown = FALSE;

    try
    {
        // m_iNumUnits = theTransports.GetNumTransports();
        m_iNumUnits = CTransportData::num_types;
        m_pwaUnits  = new WORD[m_iNumUnits];

        // m_iNumBldgs = theStructures.GetNumBuildings();
        m_iNumBldgs = CStructureData::num_types;
        m_pwaBldgs  = new WORD[m_iNumBldgs];

        m_iNumRAs = CRaceDef::num_race;
        m_pwaRAs  = new WORD[m_iNumRAs];

        m_pwaAttackedUnits = new WORD[m_iNumUnits];

        m_pwaAttackedBldgs = new WORD[m_iNumBldgs];
    }
    catch ( CException* e )
    {
        if ( m_pwaUnits != NULL )
        {
            delete[] m_pwaUnits;
            m_pwaUnits = NULL;
        }
        if ( m_pwaBldgs != NULL )
        {
            delete[] m_pwaBldgs;
            m_pwaBldgs = NULL;
        }
        if ( m_pwaRAs != NULL )
        {
            delete[] m_pwaRAs;
            m_pwaRAs = NULL;
        }
        throw( ERR_CAI_BAD_NEW );
    }

    for ( int i = 0; i < m_iNumUnits; i++ )
    {
#if AI_THREADS_ENABLED
       //  BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        m_pwaUnits[i]         = 0;
        m_pwaAttackedUnits[i] = 0;
    }

    for ( int i = 0; i < m_iNumBldgs; i++ )
    {
#if AI_THREADS_ENABLED
       //  BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        m_pwaBldgs[i]         = 0;
        m_pwaAttackedBldgs[i] = 0;
    }

    for ( int i = 0; i < m_iNumRAs; i++ )
    {
#if AI_THREADS_ENABLED
       //  BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        m_pwaRAs[i] = 0;
    }
}

BOOL CAIOpFor::IsKnown( void )
{
    return m_bKnown;
}

void CAIOpFor::SetKnown( BOOL bKnown )
{
    m_bKnown = bKnown;
}


/*
    construction,		2
    *light_truck,
    med_truck,
    heavy_truck,
    light_scout,			1
    med_scout,				2
    heavy_scout,			4
    infantry_carrier,		1
    light_tank,				2
    med_tank,				4
    heavy_tank,				6
    light_art,				2
    med_art,				4
    heavy_art,				6
    *bridging,
    light_cargo,
    med_cargo,
    *heavy_cargo,
    gun_boat,				2
    destroyer,				4
    cruiser,				6
    *battleship,
    landing_craft,			1
    infantry,				1
    rangers,				1
    marines, 				1

    barracks_2,				2
    barracks_3,				4
    command_center,			2
    embassy,
    farm,				1
    fort_1,					2
    fort_2,					4
    fort_3,					6
    heavy,					6
    light_1,				4
    light_2,				6
    lumber,				1
    oil_well,			4
    refinery,			4
    coal,				1
    iron,				1
    copper,				1
    power_1,			2
    power_2,			2
    power_3,			4
    research,			4
    repair,					6
    seaport,			4
    shipyard_1,				4
    shipyard_3,				6
    smelter,			2
    warehouse,
*/

//
// calculate the cumulative strength of this opfor
//
int CAIOpFor::GetStrengths( void )
{
    int iIndustrial = GetIndustrial( );
    int iWarfare    = GetWarfare( );
    int iCombat     = GetCombat( );
    return ( ( iIndustrial + iWarfare + iCombat ) );
}
//
// calculate this opfor's combat vehicle strength
//
int CAIOpFor::GetCombat( void )
{
#if THREADS_ENABLED
    // BUGBUG this function must yield
    myYieldThread( );
    // if( myYieldThread() == TM_QUIT )
    //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

    int iStrength = 0;
    for ( int i = 0; i < m_iNumUnits; i++ )
    {
        switch ( i )
        {
        case CTransportData::light_scout:
        case CTransportData::infantry_carrier:
        case CTransportData::landing_craft:
        case CTransportData::infantry:
        case CTransportData::rangers:
            iStrength += ( m_pwaUnits[i] * 1 );
            break;
        case CTransportData::marines:
            iStrength += ( m_pwaUnits[i] * 1 );
            break;
        case CTransportData::med_scout:
        case CTransportData::light_tank:
            iStrength += ( m_pwaUnits[i] * 1 );
            break;
        case CTransportData::light_art:
            iStrength += ( m_pwaUnits[i] * 1 );
            break;
        case CTransportData::gun_boat:
            iStrength += ( m_pwaUnits[i] * 2 );
            break;
        case CTransportData::heavy_scout:
        case CTransportData::med_tank:
            iStrength += ( m_pwaUnits[i] * 3 );
            break;
        case CTransportData::med_art:
            iStrength += ( m_pwaUnits[i] * 3 );
            break;
        case CTransportData::destroyer:
            iStrength += ( m_pwaUnits[i] * 4 );
            break;
        case CTransportData::heavy_tank:
            iStrength += ( m_pwaUnits[i] * 5 );
            break;
        case CTransportData::heavy_art:
            iStrength += ( m_pwaUnits[i] * 5 );
            break;
        case CTransportData::cruiser:
            iStrength += ( m_pwaUnits[i] * 6 );
            break;
        default:
            break;
        }
    }
    return ( iStrength );
}
//
// calculate this opfor's ability to wage warfare
//
int CAIOpFor::GetWarfare( void )
{
#if THREADS_ENABLED
    // BUGBUG this function must yield
    myYieldThread( );
    // if( myYieldThread() == TM_QUIT )
    //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

    int iStrength = 0;
    for ( int i = 0; i < m_iNumBldgs; i++ )
    {
        switch ( i )
        {
        case CStructureData::barracks_2:
        case CStructureData::command_center:
        case CStructureData::fort_1:
            iStrength += ( m_pwaBldgs[i] * 2 );
            break;
        case CStructureData::barracks_3:
        case CStructureData::fort_2:
        case CStructureData::light_1:
        case CStructureData::shipyard_1:
            iStrength += ( m_pwaBldgs[i] * 4 );
            break;
        case CStructureData::fort_3:
        case CStructureData::light_2:
        case CStructureData::repair:
        case CStructureData::shipyard_3:
        case CStructureData::heavy:
            iStrength += ( m_pwaBldgs[i] * 6 );
            break;
        default:
            break;
        }
    }
    return ( iStrength );
}
//
// calculate the industrial strength of this opfor
//
int CAIOpFor::GetIndustrial( void )
{
#if THREADS_ENABLED
    // BUGBUG this function must yield
    myYieldThread( );
    // if( myYieldThread() == TM_QUIT )
    //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

    // initialize with construction trucks
    int iStrength = ( m_pwaUnits[CTransportData::construction] * 2 );

    for ( int i = 0; i < m_iNumBldgs; i++ )
    {
        switch ( i )
        {
        case CStructureData::farm:
        case CStructureData::lumber:
        case CStructureData::coal:
        case CStructureData::iron:
        case CStructureData::copper:
            iStrength += ( m_pwaBldgs[i] * 1 );
            break;
        case CStructureData::power_1:
        case CStructureData::power_2:
        case CStructureData::smelter:
            iStrength += ( m_pwaBldgs[i] * 2 );
            break;
        case CStructureData::oil_well:
        case CStructureData::refinery:
        case CStructureData::power_3:
        case CStructureData::research:
        case CStructureData::seaport:
            iStrength += ( m_pwaBldgs[i] * 4 );
            break;
        default:
            break;
        }
    }
    return ( iStrength );
}
//
// called when non-attack messages are received, to create
// a degradation of attitude from WAR to PEACE
//
void CAIOpFor::AdjustThreat( void )
{
    // count messages received until threshhold is reached
    if ( m_iMsgCount < THREATHOLD )
    {
        m_iMsgCount++;
    }
    else  // threshhold was reached
    {
        m_iMsgCount = 0;

        // allow attitude to migrate towards PEACE
        if ( (BYTE)m_iAttitude <= PEACE )
            m_iAttitude++;

        SetRelations( );
    }
}

//
// on NULL message, get counts of opfor units
//
// otherwise update based on message type
//
void CAIOpFor::UpdateCounts( CAIMsg* pMsg )
{
    ASSERT_VALID( this );
    ASSERT_VALID( &theVehicleMap );
    ASSERT_VALID( &theBuildingMap );

    m_bKnown = TRUE;

    if ( pMsg == NULL )
    {
        // clear count of known units (vehicles)
        for ( int i = 0; i < m_iNumUnits; i++ )
        {
#if 0  // THREADS_ENABLED
       //  BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
            m_pwaUnits[i] = 0;
        }

        // first do vehicles
        POSITION pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
#if THREADS_ENABLED
            // BUGBUG this function must yield
            myYieldThread( );
            // if( myYieldThread() == TM_QUIT )
            //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
            EnterCriticalSection( &cs );

            CVehicle* pVehicle;
            DWORD     dwDumb;
            theVehicleMap.GetNextAssoc( pos, dwDumb, pVehicle );

            ASSERT_VALID( pVehicle );

            // consider only those vehicles of this player
            if ( pVehicle->GetOwner( )->GetPlyrNum( ) == m_iPlayer )
            {
                if ( pVehicle->GetData( )->GetType( ) < m_iNumUnits )
                {

                    // count this vehicle type
                    m_pwaUnits[pVehicle->GetData( )->GetType( )]++;
                }
            }
            LeaveCriticalSection( &cs );
        }

        // now do buildings

        for ( int i = 0; i < m_iNumBldgs; i++ )
        {
#if 0  // THREADS_ENABLED
       //  BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
            m_pwaBldgs[i] = 0;
        }

        pos = theBuildingMap.GetStartPosition( );
        while ( pos != NULL )
        {
#if THREADS_ENABLED
            // BUGBUG this function must yield
            myYieldThread( );
            // if( myYieldThread() == TM_QUIT )
            // throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
            EnterCriticalSection( &cs );

            // CBuilding *pBldg = theBuildingList.GetNext (pos);
            CBuilding* pBldg;
            DWORD      dwDumb;
            theBuildingMap.GetNextAssoc( pos, dwDumb, pBldg );

            ASSERT_VALID( pBldg );

            // consider only those building of this player
            if ( pBldg->GetOwner( )->GetPlyrNum( ) == m_iPlayer )
            {
                if ( pBldg->GetData( )->GetType( ) < m_iNumBldgs )
                {
                    // record the id of the opfor rocket building
                    if ( pBldg->GetData( )->GetType( ) == CStructureData::rocket && !m_dwRocket )
                        m_dwRocket = pBldg->GetID( );

                    // count this building type
                    m_pwaBldgs[pBldg->GetData( )->GetType( )]++;
                }
            }
            LeaveCriticalSection( &cs );
        }
    }
    else  // pMsg != NULL
    {
        // use pMsg data to update the sighted vehicle/building
        // or destroyed vehicle/building
        if ( pMsg->m_iMsg == CNetCmd::unit_damage )
        {
            // attitude migrates toward WAR
            if ( m_iAttitude )
                m_iAttitude--;
            // effect of difficulty level
            if ( m_iAttitude )
                m_iAttitude -= pGameData->m_iSmart;
            if ( m_iAttitude < 0 )
                m_iAttitude = 0;

            SetRelations( );

            // reset message tracking
            m_iMsgCount = 0;
        }
        WORD wCnt;
        if ( pMsg->m_iMsg == CNetCmd::veh_new )
        {
            wCnt                       = m_pwaUnits[pMsg->m_idata1];
            m_pwaUnits[pMsg->m_idata1] = wCnt + 1;
        }
        if ( pMsg->m_iMsg == CNetCmd::bldg_new )
        {
            wCnt                       = m_pwaBldgs[pMsg->m_idata1];
            m_pwaBldgs[pMsg->m_idata1] = wCnt + 1;
        }
    }
}

void CAIOpFor::SetRelations( void )
{
    // use attitude factor to set a relations state
    if ( (BYTE)m_iAttitude <= WAR )
        m_cRelations = WAR;
    else if ( (BYTE)m_iAttitude <= HOSTILE )
        m_cRelations = HOSTILE;
    else if ( (BYTE)m_iAttitude > HOSTILE && (BYTE)m_iAttitude < PEACE )
        m_cRelations = NEUTRAL;
    else if ( (BYTE)m_iAttitude >= PEACE && (BYTE)m_iAttitude < ALLIANCE )
        m_cRelations = PEACE;
    else
        m_cRelations = ALLIANCE;

    // apply attitude to 'at war' status
    if ( m_cRelations <= HOSTILE )
        m_bAtWar = TRUE;
    else
        m_bAtWar = FALSE;

    if ( m_bIsAI && pGameData->m_iSmart >= 2 )
    {
        m_bAtWar     = FALSE;
        m_cRelations = ALLIANCE;
    }
}

BYTE CAIOpFor::GetRelations( void )
{
    return m_cRelations;
}

void CAIOpFor::SetRelations( BYTE bNewRelationship )
{
    m_cRelations = bNewRelationship;
}

BOOL CAIOpFor::AtWar( void )
{
    ASSERT_VALID( this );
    return m_bAtWar;
}

void CAIOpFor::SetWar( BOOL bWar )
{
    ASSERT_VALID( this );
    m_bAtWar = bWar;
}
//
// return the flag indicating control by this AI
//
BOOL CAIOpFor::IsAI( void )
{
    ASSERT_VALID( this );
    return m_bIsAI;
}
//
// record the fact this OpFor is controlled
// by this AI
//
void CAIOpFor::SetIsAI( BOOL bAI )
{
    ASSERT_VALID( this );
    m_bIsAI = bAI;
}
//
// return the count of vehicles indicated
// by the offset passed
//
WORD CAIOpFor::GetVehicle( WORD wOffset )
{
    ASSERT_VALID( this );
    if ( m_pwaUnits != NULL )
    {
        // ASSERT_VALID( m_pwaUnits );
        // ASSERT( (int)wOffset < m_iNumUnits );

        if ( (int)wOffset < m_iNumUnits )
            return ( m_pwaUnits[wOffset] );
        else
        {
            WORD wCnt = 0;
            for ( int i = 0; i < m_iNumUnits; ++i )
            {
#if 0  // THREADS_ENABLED
       //  BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
                wCnt += m_pwaUnits[i];
            }
            return ( wCnt );
        }
    }
    return ( FALSE );
}
// BUGBUG: This needs to consider that the specific vehicle
//         or building needs to only be counted once
//
// update count of vehicles at offset passed
//
void CAIOpFor::SetVehicle( WORD wOffset, WORD wCnt )
{
    ASSERT_VALID( this );
    if ( m_pwaUnits != NULL )
    {
        // ASSERT_VALID( m_pwaUnits );
        ASSERT( (int)wOffset < m_iNumUnits );

        if ( (int)wOffset < m_iNumUnits )
            m_pwaUnits[wOffset] = wCnt;
    }
}
//
// return the racial characteristic known about this OPFOR
// as identified by the offset value passed
//
WORD CAIOpFor::GetAttribute( WORD wOffset )
{
    ASSERT_VALID( this );
    if ( m_pwaRAs != NULL )
    {
        // ASSERT_VALID( m_pwaRAs );
        ASSERT( (int)wOffset < m_iNumRAs );

        if ( (int)wOffset < m_iNumRAs )
            return ( m_pwaRAs[wOffset] );
    }
    return ( FALSE );
}
//
// set the racial characteristic known about this OPFOR
// as identified by the offset value passed
//
void CAIOpFor::SetAttribute( WORD wOffset, WORD wValue )
{
    ASSERT_VALID( this );
    if ( m_pwaRAs != NULL )
    {
        // ASSERT_VALID( m_pwaRAs );
        ASSERT( (int)wOffset < m_iNumRAs );

        if ( (int)wOffset < m_iNumRAs )
            m_pwaRAs[wOffset] = wValue;
    }
}

//
// return the count of buildings indicated
// by the offset passed
//
WORD CAIOpFor::GetBuilding( WORD wOffset )
{
    ASSERT_VALID( this );
    if ( m_pwaBldgs != NULL )
    {
        // ASSERT_VALID( m_pwaBldgs );
        ASSERT( (int)wOffset < m_iNumBldgs );

        if ( (int)wOffset < m_iNumBldgs )
            return ( m_pwaBldgs[wOffset] );
        else
        {
            WORD wCnt = 0;
            for ( int i = 0; i < m_iNumBldgs; ++i )
            {
#if 0  // THREADS_ENABLED
       //  BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
                wCnt += m_pwaBldgs[i];
            }
            return ( wCnt );
        }
    }
    return ( FALSE );
}
// BUGBUG: This needs to consider that the specific vehicle
//         or building needs to only be counted once
//
// update count of buildings at offset passed
//
void CAIOpFor::SetBuilding( WORD wOffset, WORD wCnt )
{
    ASSERT_VALID( this );
    if ( m_pwaBldgs != NULL )
    {
        // ASSERT_VALID( m_pwaBldgs );
        ASSERT( (int)wOffset < m_iNumBldgs );

        if ( (int)wOffset < m_iNumBldgs )
            m_pwaBldgs[wOffset] = wCnt;
    }
}
//
// add one to the count of attacks by this OpFor
// on this vehicle type
//
void CAIOpFor::AddUnitAttack( int iVeh )
{
    ASSERT_VALID( this );
    if ( m_pwaAttackedUnits != NULL )
    {
        // ASSERT_VALID( m_pwaAttackedUnits );
        ASSERT( iVeh < m_iNumUnits );

        if ( iVeh < m_iNumUnits )
        {
            m_pwaAttackedUnits[iVeh]++;
        }
    }
}

WORD CAIOpFor::GetTotalAttacks( void )
{
    ASSERT_VALID( this );
    WORD wCnt = 0;
    if ( m_pwaAttackedUnits != NULL )
    {
#if THREADS_ENABLED
        // BUGBUG this function must yield
        myYieldThread( );
        // if( myYieldThread() == TM_QUIT )
        //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        for ( int i = 0; i < m_iNumUnits; ++i )
        {
            wCnt += m_pwaAttackedUnits[i];
        }
    }
    if ( m_pwaAttackedBldgs != NULL )
    {
#if THREADS_ENABLED
        // BUGBUG this function must yield
        myYieldThread( );
        // if( myYieldThread() == TM_QUIT )
        //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        for ( int i = 0; i < m_iNumBldgs; ++i )
        {
            wCnt += m_pwaAttackedBldgs[i];
        }
    }
    return ( wCnt );
}

//
// get count of attacks by this OpFor by vehicle type
//
WORD CAIOpFor::GetUnitAttack( int iVeh )
{
    ASSERT_VALID( this );
    if ( m_pwaAttackedUnits != NULL )
    {
        // ASSERT_VALID( m_pwaAttackedUnits );
        // ASSERT( iVeh < m_iNumUnits );

        if ( iVeh < m_iNumUnits )
        {
            return ( m_pwaAttackedUnits[iVeh] );
        }
        else
        {
            WORD wCnt = 0;
            for ( int i = 0; i < m_iNumUnits; ++i )
            {
#if 0  // THREADS_ENABLED
       //  BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
                wCnt += m_pwaAttackedUnits[i];
            }
            return ( wCnt );
        }
    }
    return ( 0 );
}

//
// add one to the count of attacks by this OpFor
// on this building type
//
void CAIOpFor::AddBldgAttack( int iBldg )
{
    ASSERT_VALID( this );
    if ( m_pwaAttackedBldgs != NULL )
    {
        // ASSERT_VALID( m_pwaAttackedBldgs );
        ASSERT( iBldg < m_iNumBldgs );

        if ( iBldg < m_iNumBldgs )
        {
            m_pwaAttackedBldgs[iBldg]++;
        }
    }
}

//
// get count of attacks by this OpFor by building type
//
WORD CAIOpFor::GetBldgAttack( int iBldg )
{
    ASSERT_VALID( this );
    if ( m_pwaAttackedBldgs != NULL )
    {
        // ASSERT_VALID( m_pwaAttackedBldgs );
        // ASSERT( iBldg < m_pwaAttackedBldgs->GetSize() );

        if ( iBldg < m_iNumBldgs )
        {
            return ( m_pwaAttackedBldgs[iBldg] );
        }
        else
        {
            WORD wCnt = 0;
            for ( int i = 0; i < m_iNumBldgs; ++i )
            {
#if 0  // THREADS_ENABLED
       //  BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
                wCnt += m_pwaAttackedBldgs[i];
            }
            return ( wCnt );
        }
    }
    return ( 0 );
}

int CAIOpFor::GetPlayerID( void )
{
    ASSERT_VALID( this );
    return m_iPlayer;
}


DWORD CAIOpFor::GetRocket( void )
{
    ASSERT_VALID( this );
    return m_dwRocket;
}

int CAIOpFor::GetMsgCount( void )
{
    ASSERT_VALID( this );
    return m_iMsgCount;
}

void CAIOpFor::SetRocket( DWORD dwRocket )
{
    ASSERT_VALID( this );
    m_dwRocket = dwRocket;
}


void CAIOpFor::SetMsgCount( int iCnt )
{
    ASSERT_VALID( this );
    m_iMsgCount = iCnt;
}

int CAIOpFor::GetAttitude( void )
{
    ASSERT_VALID( this );
    return m_iAttitude;
}

void CAIOpFor::SetAttitude( int iAttitude )
{
    ASSERT_VALID( this );
    if ( iAttitude > 0 && (BYTE)iAttitude <= ALLIANCE )
        m_iAttitude = iAttitude;
}

//
// return the name of the CAIOpFor
//
CString& CAIOpFor::GetName( void )
{
    ASSERT_VALID( this );
    return m_sName;
}
CAIOpFor::~CAIOpFor( )
{
    ASSERT_VALID( this );


    if ( m_pwaUnits != NULL )
    {
        delete[] m_pwaUnits;
        m_pwaUnits = NULL;
    }
    if ( m_pwaBldgs != NULL )
    {
        delete[] m_pwaBldgs;
        m_pwaBldgs = NULL;
    }
    if ( m_pwaRAs != NULL )
    {
        delete[] m_pwaRAs;
        m_pwaRAs = NULL;
    }
    if ( m_pwaAttackedUnits != NULL )
    {
        delete[] m_pwaAttackedUnits;
        m_pwaAttackedUnits = NULL;
    }
    if ( m_pwaAttackedBldgs != NULL )
    {
        delete[] m_pwaAttackedBldgs;
        m_pwaAttackedBldgs = NULL;
    }
    m_sName.Empty( );
}

void CAIOpFor::Save( CFile* pFile )
{
    pFile->Write( (const void*)m_pwaUnits, ( sizeof( WORD ) * m_iNumUnits ) );
    pFile->Write( (const void*)m_pwaBldgs, ( sizeof( WORD ) * m_iNumBldgs ) );
    pFile->Write( (const void*)m_pwaRAs, ( sizeof( WORD ) * m_iNumRAs ) );
    pFile->Write( (const void*)m_pwaAttackedUnits, ( sizeof( WORD ) * m_iNumUnits ) );
    pFile->Write( (const void*)m_pwaAttackedBldgs, ( sizeof( WORD ) * m_iNumBldgs ) );
}

void CAIOpFor::Load( CFile* pFile )
{
    pFile->Read( (void*)m_pwaUnits, ( sizeof( WORD ) * m_iNumUnits ) );
    pFile->Read( (void*)m_pwaBldgs, ( sizeof( WORD ) * m_iNumBldgs ) );
    pFile->Read( (void*)m_pwaRAs, ( sizeof( WORD ) * m_iNumRAs ) );
    pFile->Read( (void*)m_pwaAttackedUnits, ( sizeof( WORD ) * m_iNumUnits ) );
    pFile->Read( (void*)m_pwaAttackedBldgs, ( sizeof( WORD ) * m_iNumBldgs ) );
}

/////////////////////////////////////////////////////////////////////////////

CAIOpForList::~CAIOpForList( void )
{
    ASSERT_VALID( this );
    DeleteList( );
}

#if 0
WORD CAIOpForList::GetNextId( void )
{
	ASSERT_VALID( this );
	
    if( GetCount() )
    {
        CWordArray *pIds = NULL;
        
        try
        {
        	pIds = new CWordArray;
            pIds->SetSize( MAX_OPFORS, 1 );
        }
        catch( CException e )
        {
        	if( pIds != NULL )
        	{
        		pIds->RemoveAll();
            	delete pIds;
            }
            throw(ERR_CAI_BAD_NEW);
        }
        
        // initialize offset 0 as used and all others as unused
        for( int i=0; i<pIds->GetSize(); ++i )
            pIds->SetAt( i, FALSE );
        pIds->SetAt( 0, TRUE );
        
        POSITION pos = GetHeadPosition();
        while( pos != NULL )
        {   
            CAIOpFor *pOpFor = (CAIOpFor *)GetNext( pos );
            if( pOpFor != NULL )
            {
            	ASSERT_VALID( pOpFor );

            	if( (int)pOpFor->GetID() < pIds->GetSize() )
            		pIds->SetAt( pOpFor->GetID(), TRUE );
            } 
        }
        
        // now find the first FALSE indicating the lowest unused id
        WORD wNextId = FALSE;
        for( i=1; i<pIds->GetSize(); ++i )
        {
            if( !pIds->GetAt(i) )
            {
                wNextId = i;
                break;
            }
        }
        pIds->RemoveAll();
        delete pIds;
        return( wNextId );
    }
    else
    	return(1);
}
#endif

CAIOpFor* CAIOpForList::AddOpFor( int iPlayer )
{
    ASSERT_VALID( this );

    CAIOpFor* pOpFor = NULL;

    try
    {
        pOpFor = new CAIOpFor( iPlayer, NULL );
        AddTail( (CObject*)pOpFor );
    }
    catch ( CException* e )
    {
        pOpFor = NULL;

        throw( ERR_CAI_BAD_NEW );
    }

    // access the game data for this player
    EnterCriticalSection( &cs );
    CPlayer* pPlayer = pGameData->GetPlayerData( iPlayer );
    if ( pPlayer == NULL )
    {
        // for some reason this player is gone now
        LeaveCriticalSection( &cs );

        RemoveOpFor( iPlayer );
        delete pOpFor;

        throw( ERR_CAI_BAD_NEW );
    }
    // record if this player is AI or not
    pOpFor->SetIsAI( pPlayer->IsAI( ) );
    LeaveCriticalSection( &cs );

    return ( pOpFor );
}

//
// return a pointer to the CAIOpFor object matching the id passed
//
CAIOpFor* CAIOpForList::GetOpFor( int iPlayer )
{
    ASSERT_VALID( this );

#if THREADS_ENABLED
    // BUGBUG this function must yield
    myYieldThread( );
    // if( myYieldThread() == TM_QUIT )
    //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

    POSITION pos = GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIOpFor* pOpFor = (CAIOpFor*)GetNext( pos );
        if ( pOpFor != NULL )
        {
            ASSERT_VALID( pOpFor );

            if ( pOpFor->GetPlayerID( ) == iPlayer )
                return ( pOpFor );
        }
    }
    return ( NULL );
}

//
// delete CAIOpFor objects in the list
//
void CAIOpForList::DeleteList( void )
{
    ASSERT_VALID( this );

    if ( GetCount( ) )
    {
        POSITION pos = GetHeadPosition( );
        while ( pos != NULL )
        {
            CAIOpFor* pOpFor = (CAIOpFor*)GetNext( pos );
            if ( pOpFor != NULL )
            {
                ASSERT_VALID( pOpFor );

                delete pOpFor;
            }
        }
    }
    RemoveAll( );
}

void CAIOpForList::RemoveOpFor( int iPlayer )
{
    ASSERT_VALID( this );

    POSITION pos1, pos2;
    for ( pos1 = GetHeadPosition( ); ( pos2 = pos1 ) != NULL; )
    {
#if 0  // THREADS_ENABLED
       //  BUGBUG this function must yield
			myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        CAIOpFor* pOpFor = (CAIOpFor*)GetNext( pos1 );
        if ( pOpFor != NULL )
        {
            ASSERT_VALID( pOpFor );

            if ( pOpFor->GetPlayerID( ) != iPlayer )
                continue;
        }

        pOpFor = (CAIOpFor*)GetAt( pos2 );
        if ( pOpFor != NULL )
        {
            ASSERT_VALID( pOpFor );

            RemoveAt( pos2 );
            delete pOpFor;
            break;
        }
    }
}

WORD CAIOpForList::GetBuildingCnt( int iBldg )
{
    WORD wCnt = 0;
    if ( iBldg >= CStructureData::num_types || iBldg < 0 )
        return ( wCnt );

    if ( GetCount( ) )
    {
        POSITION pos = GetHeadPosition( );
        while ( pos != NULL )
        {
#if 0  // THREADS_ENABLED
       //  BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

            CAIOpFor* pOpFor = (CAIOpFor*)GetNext( pos );
            if ( pOpFor != NULL )
            {
                ASSERT_VALID( pOpFor );

                wCnt += pOpFor->GetBuilding( iBldg );
            }
        }
    }
    return ( wCnt );
}

//
// this processes all OpFors in this list and sums
// the vehicle counts for the indicated vehicle
// and returns the count
//
WORD CAIOpForList::GetVehicleCnt( int iVeh, BOOL bAve /*=FALSE*/ )
{
    WORD wCnt = 0;

    if ( GetCount( ) )
    {
        POSITION pos = GetHeadPosition( );
        while ( pos != NULL )
        {
#if 0  // THREADS_ENABLED
       //  BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

            CAIOpFor* pOpFor = (CAIOpFor*)GetNext( pos );
            if ( pOpFor != NULL )
            {
                ASSERT_VALID( pOpFor );

                if ( iVeh < CTransportData::num_types )
                    wCnt += pOpFor->GetVehicle( iVeh );
                else
                {
                    for ( int i = 0; i < CTransportData::num_types; ++i )
                    {
                        wCnt += pOpFor->GetVehicle( i );
                    }
                }
            }
        }

        // if at least one of that type of vehicle and the
        // average count switch is set, then add 1 for each opfor
        if ( bAve && wCnt )
            wCnt += GetCount( );
    }

    // take an average
    if ( bAve && GetCount( ) )
        wCnt /= GetCount( );

    return ( wCnt );
}

//
// return count of attacks, by all opfors, based on
// iTypeUnit indicating building/vehicle and iType
// the specific building/vehicle type, or total all
// attacks if both values are invalid
//
WORD CAIOpForList::GetAttackCnt( int iType, int iTypeUnit )
{
    WORD wCnt = 0;
    if ( GetCount( ) )
    {
        POSITION pos = GetHeadPosition( );
        while ( pos != NULL )
        {

            CAIOpFor* pOpFor = (CAIOpFor*)GetNext( pos );
            if ( pOpFor != NULL )
            {
                ASSERT_VALID( pOpFor );

                if ( iTypeUnit == CUnit::vehicle )
                {
                    wCnt += pOpFor->GetUnitAttack( iType );
                }
                else if ( iTypeUnit == CUnit::building )
                {
                    wCnt += pOpFor->GetBldgAttack( iType );
                }
                else
                {
                    wCnt += pOpFor->GetTotalAttacks( );
                }
            }
        }
    }
    return ( wCnt );
}

CAIOpFor* CAIOpForList::GetNearest( CHexCoord& hexFrom, BOOL bKnown )
{
    CHexCoord hexCity;
    CAIOpFor* pBest = NULL;
    int       iDist;
    int       iBest = 0xFFFE;

    if ( GetCount( ) )
    {
        POSITION pos = GetHeadPosition( );
        while ( pos != NULL )
        {
#if THREADS_ENABLED
            // BUGBUG this function must yield
            myYieldThread( );
            // if( myYieldThread() == TM_QUIT )
            //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
            CAIOpFor* pOpFor = (CAIOpFor*)GetNext( pos );
            if ( pOpFor != NULL )
            {
                ASSERT_VALID( pOpFor );

                if ( bKnown )
                {
                    if ( !pOpFor->IsKnown( ) )
                        continue;
                }

                hexCity = hexFrom;
                pGameData->FindBuilding( CStructureData::rocket, pOpFor->GetPlayerID( ), hexCity );
                iDist = pGameData->GetRangeDistance( hexFrom, hexCity );
                if ( iDist && iDist < iBest )
                {
                    iBest = iDist;
                    pBest = pOpFor;
                }
            }
        }
    }

#ifdef SPECTATE
    // skip human players when spectating
    if ( pOpFor->isAI( ) == FALSE )
        continue;
#endif

    return ( pBest );
}

//
// get a pointer to the opfor with the lowest overall strength
//
CAIOpFor* CAIOpForList::GetWeakest( BOOL bKnown )
{
    int       iBest = 0xFFFE;
    int       iStrength;
    CAIOpFor* pBest = NULL;

    if ( GetCount( ) )
    {
        POSITION pos = GetHeadPosition( );
        while ( pos != NULL )
        {
#if AI_THREADS_ENABLED
       //  BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

            CAIOpFor* pOpFor = (CAIOpFor*)GetNext( pos );
            if ( pOpFor != NULL )
            {
                ASSERT_VALID( pOpFor );

                if ( bKnown )
                {
                    if ( !pOpFor->IsKnown( ) )
                        continue;
                }
#ifdef SPECTATE
                // skip human players when spectating
                if ( pOpFor ->isAI() == FALSE )
                    continue;
#endif

                iStrength = pOpFor->GetStrengths( );
                if ( iStrength < iBest )
                {
                    iBest = iStrength;
                    pBest = pOpFor;
                }
            }
        }
    }
    return ( pBest );
}

BOOL CAIOpForList::AllKnown( void )
{
    if ( GetCount( ) )
    {
        POSITION pos = GetHeadPosition( );
        while ( pos != NULL )
        {
#if AI_THREADS_ENABLED
       //  BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

            CAIOpFor* pOpFor = (CAIOpFor*)GetNext( pos );
            if ( pOpFor != NULL )
            {
                ASSERT_VALID( pOpFor );

                if ( !pOpFor->IsKnown( ) )
                    return FALSE;
            }
        }
    }
    return TRUE;
}

void CAIOpForList::Save( CFile* pFile )
{
    int iCnt = GetCount( );
    try
    {
        pFile->Write( (const void*)&iCnt, sizeof( int ) );
    }
    catch ( CFileException* theException )
    {
        // how should write errors be reported?
        throw( ERR_CAI_BAD_FILE );
    }

    if ( iCnt )
    {
        OpForBuff ofb;

        POSITION pos = GetHeadPosition( );
        while ( pos != NULL )
        {
            CAIOpFor* pOpFor = (CAIOpFor*)GetNext( pos );
            if ( pOpFor != NULL )
            {
                ofb.iPlayer    = pOpFor->GetPlayerID( );
                ofb.dwRocket   = pOpFor->GetRocket( );
                ofb.iMsgCount  = pOpFor->GetMsgCount( );
                ofb.iAttitude  = pOpFor->GetAttitude( );
                ofb.cRelations = pOpFor->GetRelations( );
                ofb.bAtWar     = pOpFor->AtWar( );
                ofb.bIsAI      = pOpFor->IsAI( );
                ofb.bIsKnown   = pOpFor->IsKnown( );

                pFile->Write( (const void*)&ofb, sizeof( OpForBuff ) );

                // BUGBUG these arrays are all protected so the opfor list can't see
                // them and will need to inerate throught the get*() of the opfor
                pOpFor->Save( pFile );
            }
        }
    }
}

void CAIOpForList::Load( CFile* pFile )
{
    // make any old opfors go away
    DeleteList( );

    OpForBuff ofb;

    // now get count of opfors
    int iCnt;
    pFile->Read( (void*)&iCnt, sizeof( int ) );

    if ( iCnt )
    {
        int iBytes;
        for ( int i = 0; i < iCnt; ++i )
        {
            // read file data into buffer
            iBytes = pFile->Read( (void*)&ofb, sizeof( OpForBuff ) );

            // BUGBUG how should read errors be reported?
            if ( iBytes != sizeof( OpForBuff ) )
                return;

            CAIOpFor* pOpFor = NULL;
            try
            {
                // CAIOpFor( int iPlayer, const char *pzName )
                pOpFor = new CAIOpFor( ofb.iPlayer, "" );
            }
            catch ( CException* anException )
            {
                // how should memory errors be reported?
                throw( ERR_CAI_BAD_NEW );
            }
            pOpFor->SetRocket( ofb.dwRocket );
            pOpFor->SetMsgCount( ofb.iMsgCount );
            pOpFor->SetAttitude( ofb.iAttitude );
            pOpFor->SetRelations( ofb.cRelations );
            pOpFor->SetWar( ofb.bAtWar );
            pOpFor->SetIsAI( ofb.bIsAI );
            pOpFor->SetKnown( ofb.bIsKnown );

            // BUGBUG these arrays are all protected so the opfor list can't see
            // them and will need to inerate throught the get*() of the opfor
            pOpFor->Load( pFile );

            // put the opfor in the list
            AddTail( (CObject*)pOpFor );
        }
    }

    if ( GetCount( ) )
    {
        POSITION pos = GetHeadPosition( );
        while ( pos != NULL )
        {
            CAIOpFor* pOpFor = (CAIOpFor*)GetNext( pos );
            if ( pOpFor != NULL )
            {
                ASSERT_VALID( pOpFor );
                CPlayer* pPlayer = pGameData->GetPlayerData( pOpFor->GetPlayerID( ) );

                // confirm this player is AI or not in
                // case an HP switched players
                if ( pPlayer != NULL )
                    pOpFor->SetIsAI( pPlayer->IsAI( ) );
            }
        }
    }
}

// end of CAIOpFor.cpp
