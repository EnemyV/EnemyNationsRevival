//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// terrain.cpp : the hexes & terrain
//

#include "ai.h"
#include "help.h"
#include "lastplnt.h"
#include "minerals.inl"
#include "player.h"
#include "stdafx.h"
#include "terrain.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


const int OCEAN_COAST_OFF = 0;
const int LAKE_COAST_OFF  = 13;
const int RIVER_COAST_OFF = 26;


/////////////////////////////////////////////////////////////////////////////
// CHex - a single hex

CHex::CHex( )
{

    m_bType    = 0;  // CHex::plain;
    m_bAlt     = unassigned;
    m_bUnit    = 0;
    m_bVisible = 0;

    SetVisibleType( 0 );

#ifdef _CHEAT
    if ( _bSeeAll )
        m_bVisible = 1;
#endif
}

// we limit/force hills and mountains based on the slope
void CHex::SetType( int iType )
{

    ASSERT_STRICT( ( 0 <= iType ) && ( iType < CHex::num_types ) );

    // if not land - just do it
    if ( ( iType == city ) || ( iType == road ) || IsWater( ) )
    {
        SetVisibleType( iType );
        m_bType = (BYTE)( ( m_bType & 0xF0 ) | ( iType & 0x0F ) );
        InitType( );
        return;
    }

    CHexCoord _hex( GetHex( ) );

    CHex* pHexUR    = theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) - 1 );
    CHex* pHexTop   = theMap.GetHex( _hex.X( ), _hex.Y( ) - 1 );
    CHex* pHexRight = theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) );

    int iAltMax, iAltMin;
    iAltMax = iAltMin = GetAlt( );
    iAltMin           = __min( iAltMin, pHexUR->GetAlt( ) );
    iAltMin           = __min( iAltMin, pHexTop->GetAlt( ) );
    iAltMin           = __min( iAltMin, pHexRight->GetAlt( ) );
    iAltMax           = __max( iAltMax, pHexUR->GetAlt( ) );
    iAltMax           = __max( iAltMax, pHexTop->GetAlt( ) );
    iAltMax           = __max( iAltMax, pHexRight->GetAlt( ) );

    if ( iAltMin != 0 )
    {
        int iSlope = iAltMax - iAltMin;

        // mountain: force if > 15, allow if > 8
        if ( ( iSlope > 15 ) || ( ( iSlope > 8 ) && ( iType == mountain ) ) )
            iType = mountain;
        else
            // hill: force if > 8, allow if > 4
            if ( ( iSlope > 8 ) || ( ( iSlope > 4 ) && ( iType == hill ) ) )
            iType = hill;
        else
            // if mountain - make it rough
            if ( iType == mountain )
            iType = rough;
        else
            // if hill - rand rough/plains
            if ( iType == hill )
        {
            if ( MyRand( ) & 0x0100 )
                iType = rough;
            else
                iType = plain;
        }
    }

    SetVisibleType( iType );
    m_bType = (BYTE)( ( m_bType & 0xF0 ) | ( iType & 0x0F ) );

    InitType( );
}

// we change the altitude based on how far apart items are
static int ConvertAlt( int iAlt, int iSideSize )
{

    int iDiff = ( iAlt - CHex::sea_level );
    return ( CHex::sea_level + ( iDiff / 4 ) + ( iDiff * iSideSize ) / 128 );
}

void CHex::Init( int iAlt )
{

    // if we're overriding a prev assignment, higher alt wins
    // unassigned == 0 so it always works
    if ( GetAlt( ) < iAlt )
        SetAlt( iAlt );
}

void CHex::SetOceanAlt( int iAlt )
{

    SetAlt( iAlt );

    int iIndex = RandNum( theTerrain.GetCount( ocean ) - 1 );
    m_psprite  = theTerrain.GetSprite( ocean, iIndex );
}

// FIXIT: Upgrade when new art available
void CHex::InitType( )
{

    int iIndex = 0;
    int iNum   = theTerrain.GetCount( GetType( ) );
    if ( iNum > 1 )
        iIndex = RandNum( iNum - 1 );

    // assign the sprite
    m_psprite = theTerrain.GetSprite( GetType( ), iIndex );
}

/////////////////////////////////////////////////////////////////////////////
// CGameMap - the collection of hexes

CGameMap::CGameMap( )
{

    m_pHex       = NULL;
    m_iSideShift = 5;
    m_iHexMask   = 31;
    m_iWidthHalf = 16;
    m_iSubMask   = 63;
    m_iLocMask   = MAX_HEX_HT * 32 - 1;
    m_iLocHalf   = 16 * MAX_HEX_HT;
    m_iBldgCur   = CHex::no_cur;
    m_cxBldgCur = m_cyBldgCur = 0;
    m_pLandExit = m_pShipExit = NULL;
    m_iLandDir = m_iShipDir = 0;
}

void CGameMap::Close( )
{

    if ( m_pHex == NULL )
        return;

    delete[] m_pHex;
    m_pHex = NULL;

    m_eX = m_eY = 0;
}

void CGameMap::GetWorldSize( int iSize, int& iSide, int& iSideSize )
{

    // add 2 blocks for each island & .25 for each ocean-front
    // add 1 block total to liven things up
    float fNumBlks = (float)theGame.GetAll( ).GetCount( ) - 0.4f;

#ifdef _CHEAT
    if ( ( theGame.GetServerNetNum( ) == 0 ) && ( theApp.GetProfileInt( "Cheat", "ForceOcean", 0 ) ) )
        fNumBlks += 2.0f;
#endif

    // scenarios force an ocean
    if ( theGame.GetScenario( ) >= 0 )
        fNumBlks += 4.0f;
    else
        fNumBlks += RandNum( 4 ); // randomly add 4 blocks? interesting

    // we're adding blocks for more island and ocean players
    // interestingly this means the map size is dependant on the kind of races
    // I did notice this, some races had smaller planets

    POSITION pos;
    for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
        ASSERT_VALID( pPlr );
        if ( pPlr->m_InitData.GetSupplies( CRaceDef::island ) )  
            fNumBlks += 2;
        else if ( pPlr->m_InitData.GetSupplies( CRaceDef::ocean ) ) 
            fNumBlks += 0.25f;
    }

    // determine the size of the world
    int iMin = (int)sqrt( fNumBlks ) + 1;
    iSide    = 1;
    while ( iSide < iMin ) iSide *= 2;

    ASSERT( (float)( iSide * iSide ) >= fNumBlks );
    iSideSize;
    switch ( iSize )
    {
    case 1:
        iSideSize = MIN_SIDE_SIZE << 1;
        break;
    case 2:
        iSideSize = MAX_SIDE_SIZE;
        break;
    default:
        iSideSize = MIN_SIDE_SIZE;
        break;
    }
}

static int IndPrev( int iInd, int iSide )
{

    if ( iInd % iSide != 0 )
        return ( iInd - 1 );

    return ( iInd + iSide - 1 );
}

static int IndNext( int iInd, int iSide )
{

    iInd++;
    if ( iInd % iSide != 0 )
        return ( iInd );

    return ( iInd - iSide );
}

static int IndLeft( int iInd, int iSide )
{

    iInd -= iSide;
    if ( iInd >= 0 )
        return ( iInd );

    return ( iInd + iSide * iSide );
}

static int IndRight( int iInd, int iSide )
{

    iInd += iSide;
    int iBlk = iSide * iSide;
    if ( iInd >= iBlk )
        return ( iInd - iBlk );

    return ( iInd );
}

static int fnEnumIncVis( CHex* pHex, CHexCoord, void* )
{

    pHex->IncVisible( );

    return ( FALSE );
}

void CGameMap::Init( int iSide, int iSideSize, int iScenario )
{

    theApp.m_pCreateGame->GetDlgStatus( )->SetMsg( IDS_ALLOC_MAP );
    theApp.m_pCreateGame->GetDlgStatus( )->SetPer( PER_WORLD_START );

    m_iBldgCur  = CHex::no_cur;
    m_cxBldgCur = m_cyBldgCur = 0;
    m_pLandExit = m_pShipExit = NULL;
    m_iLandDir = m_iShipDir = 0;

    m_eX        = iSide * iSideSize;
    m_eY        = iSide * iSideSize;
    m_iSideSize = iSideSize;

    int iTmp     = m_eX;
    m_iSideShift = 0;
    while ( iTmp > 1 )
    {
        iTmp >>= 1;
        m_iSideShift++;
    }

    // Create parallel bit-matrix for hex invalidating
    // note - also in Serialize
    m_ptrhexvalidmatrix = new CHexValidMatrix( m_iSideShift - 1, m_iSideShift - 1 );

    // this makes use of the face eX == eY and that it's a multiple of 32
    m_iHexMask   = m_eX - 1;
    m_iWidthHalf = m_eX / 2;
    m_iSubMask   = ( m_eX * 2 ) - 1;
    m_iLocMask   = m_eX * MAX_HEX_HT - 1;
    m_iLocHalf   = m_eX * MAX_HEX_HT / 2;

    // alloc the map (we allow an extra line for some tricks I pull)
    //   note: I dup this at the end of this function
    long lTotal = (long)m_eX * (long)m_eY;
    m_pHex      = new CHex[lTotal + m_eX + 2];

    // assign players to sub-units in the map
    int  iNumBlks    = iSide * iSide;
    int  iOceansLeft = iNumBlks - theGame.GetAll( ).GetCount( );
    int* piBlks      = new int[iNumBlks];
    for ( int iInd = 0; iInd < iNumBlks; iInd++ ) piBlks[iInd] = 0;

    // we need to know the number of island requesting players
    POSITION pos;
    int      iIslandPlayersLeft = 0;
    for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
        ASSERT_VALID( pPlr );
        if ( pPlr->m_InitData.GetSupplies( CRaceDef::island ) )
            iIslandPlayersLeft++;
    }
#ifdef _CHEAT
    // Specifically force an ocean?
    // strange because it subtracts?
    if ( ( theGame.GetServerNetNum( ) == 0 ) && ( theApp.GetProfileInt( "Cheat", "ForceOcean", 0 ) ) )
    {
        piBlks[0] = -1;
        if ( iOceansLeft > 0 )
            iOceansLeft--;
    }
#endif

    // scenarios force an ocean
    if ( theGame.GetScenario( ) >= 0 )
    {
        piBlks[0] = -1;
        if ( iOceansLeft > 0 )
            iOceansLeft--;
    }

    theApp.m_pCreateGame->GetDlgStatus( )->SetPer( PER_WORLD_BLKS );

    // we walk through, if they are an island we put in 2 oceans. If they
    // are ocean front we put in one if they don't have one. We may run
    // out of oceans - tough, nothing in life is for certain!!!
    // when we're all done, remaining plots are set to desert/swamp or ocean
    int iInd       = 0;
    int iPlyrsLeft = theGame.GetAll( ).GetCount( );
    for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
    {
        // skip to the next blk
        while ( piBlks[iInd] != 0 )
        {
            if ( iInd >= iNumBlks - iPlyrsLeft )
                break;
            iInd++;
        }

        // if we are out of blks - find one!!!
        if ( iInd >= iNumBlks )
        {
            iInd = iNumBlks - 1;
            // find a free one
            while ( ( iInd > 0 ) && ( piBlks[iInd] > 0 ) ) iInd--;
            iOceansLeft = 0;
        }

        CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
        ASSERT_VALID( pPlr );

        if ( pPlr->m_InitData.GetSupplies( CRaceDef::island ) )
        {
            // if we have enough oceans, check above & to the left
            // and below (for bottom of column)
            if ( iOceansLeft > iIslandPlayersLeft * 2 - 1 )
            {
                if ( iInd < iNumBlks - 1 )
                    if ( ( piBlks[IndLeft( iInd, iSide )] > 0 ) || ( piBlks[IndPrev( iInd, iSide )] > 0 ) ||
                         ( piBlks[IndNext( iInd, iSide )] > 0 ) )
                    {
                        ASSERT( iInd < iNumBlks );
                        int iOldInd    = iInd;
                        piBlks[iInd++] = -1;
                        iOceansLeft--;

                        // skip any possible assigned tiles
                        while ( piBlks[iInd] != 0 )
                        {
                            if ( iInd >= iNumBlks - iPlyrsLeft )
                            {
                                iInd = iOldInd;
                                break;
                            }
                            iInd++;
                        }
                    }
            }

            // assign this one
            pPlr->ai.hex = pPlr->m_hexMapStart =
                CHexCoord( ( iInd / iSide ) * iSideSize + iSideSize / 2, ( iInd % iSide ) * iSideSize + iSideSize / 2 );
            ASSERT( iInd < iNumBlks );
            piBlks[iInd] = pPlr->GetPlyrNum( );
            iPlyrsLeft--;
            iIslandPlayersLeft--;

            // we now drop an ocean to the right and below
            if ( ( iOceansLeft >= iIslandPlayersLeft ) && ( iOceansLeft >= 2 ) )
            {
                if ( piBlks[IndNext( iInd, iSide )] == 0 )
                {
                    piBlks[IndNext( iInd, iSide )] = -1;
                    iOceansLeft--;
                }
                if ( piBlks[IndRight( iInd, iSide )] == 0 )
                {
                    piBlks[IndRight( iInd, iSide )] = -1;
                    iOceansLeft--;
                }
            }

            // we assigned [iInd] to the player
            iInd++;
        }

        else
        {
            // ok, its not an island - put it here
            pPlr->ai.hex = pPlr->m_hexMapStart =
                CHexCoord( ( iInd / iSide ) * iSideSize + iSideSize / 2, ( iInd % iSide ) * iSideSize + iSideSize / 2 );
            ASSERT( iInd < iNumBlks );
            piBlks[iInd++] = pPlr->GetPlyrNum( );
            iPlyrsLeft--;

            if ( ( iOceansLeft > 0 ) && ( pPlr->m_InitData.GetSupplies( CRaceDef::ocean ) ) )
            {
                BOOL bOk = FALSE;
                // see if we're already touching an ocean or island
                //   (remember, iInd-1 is what we just assigned)
                // above
                int iTest = IndPrev( iInd - 1, iSide );
                ASSERT( ( 0 <= iTest ) && ( iTest < iNumBlks ) );
                if ( piBlks[iTest] == -1 )
                    bOk = TRUE;
                else if ( piBlks[iTest] > 0 )
                {
                    if ( theGame.GetPlayerByPlyr( piBlks[iTest] )->m_InitData.GetSupplies( CRaceDef::island ) )
                    {
                        TRAP( );
                        bOk = TRUE;
                    }
                }
                // left
                iTest = IndLeft( iInd - 1, iSide );
                ASSERT( ( 0 <= iTest ) && ( iTest < iNumBlks ) );
                if ( piBlks[iTest] == -1 )
                    bOk = TRUE;
                else if ( piBlks[iTest] > 0 )
                    if ( theGame.GetPlayerByPlyr( piBlks[iTest] )->m_InitData.GetSupplies( CRaceDef::island ) )
                        bOk = TRUE;
                // below
                iTest = IndNext( iInd - 1, iSide );
                ASSERT( ( 0 <= iTest ) && ( iTest < iNumBlks ) );
                if ( piBlks[iTest] == -1 )
                    bOk = TRUE;
                else if ( piBlks[iTest] > 0 )
                    if ( theGame.GetPlayerByPlyr( piBlks[iTest] )->m_InitData.GetSupplies( CRaceDef::island ) )
                        bOk = TRUE;
                // right (only possible if it wrapped to the begining)
                iTest = IndRight( iInd - 1, iSide );
                ASSERT( ( 0 <= iTest ) && ( iTest < iNumBlks ) );
                if ( piBlks[iTest] == -1 )
                    bOk = TRUE;
                else if ( piBlks[iTest] > 0 )
                    if ( theGame.GetPlayerByPlyr( piBlks[iTest] )->m_InitData.GetSupplies( CRaceDef::island ) )
                    {
                        TRAP( );
                        bOk = TRUE;
                    }

                if ( !bOk )
                {
                    // we put it below if there is a below AND there is no ocean to the left
                    if ( ( piBlks[iInd] == 0 ) && ( iInd % iSide != 0 ) && ( piBlks[IndLeft( iInd, iSide )] != -1 ) )
                    {
                        ASSERT( iInd < iNumBlks );
                        piBlks[iInd++] = -1;  // below
                        iOceansLeft--;
                    }
                    else if ( piBlks[IndRight( iInd - 1, iSide )] == 0 )
                    {
                        piBlks[IndRight( iInd - 1, iSide )] = -1;
                        iOceansLeft--;
                    }
                }
            }
        }


        // first we test for an island below or to our right. If we have one
        // we want to drop an ocean if at all possible.
        if ( ( iInd < iNumBlks ) && ( piBlks[iInd] == 0 ) && ( iOceansLeft > iIslandPlayersLeft * 2 - 1 ) )
        {
            int  iTestBelow = piBlks[IndNext( iInd, iSide )];
            int  iTestRight = piBlks[IndRight( iInd, iSide )];
            int  iTestLeft  = piBlks[IndLeft( iInd, iSide )];
            BOOL bOcean     = FALSE;
            if ( iTestBelow > 0 )
                if ( theGame.GetPlayerByPlyr( iTestBelow )->m_InitData.GetSupplies( CRaceDef::island ) )
                    bOcean = TRUE;
            if ( iTestRight > 0 )
                if ( theGame.GetPlayerByPlyr( iTestRight )->m_InitData.GetSupplies( CRaceDef::island ) )
                    bOcean = TRUE;
            if ( iTestLeft > 0 )
                if ( theGame.GetPlayerByPlyr( iTestLeft )->m_InitData.GetSupplies( CRaceDef::island ) )
                    bOcean = TRUE;
            if ( bOcean )
            {
                piBlks[iInd++] = -1;
                iOceansLeft--;
            }
        }

        // if we have more oceans than 2* num island players left, we drop
        // an extra block. If we are touching an island player (only possible
        // to the left or below) we drop an ocean. If our next iInd is an
        // island and we are NOT at the bottom of a col, we drop an ocean.
        // Otherwise we drop a desert/swamp
        // +1 - we keep one last ocean for the final player - in case ocean front

        // we may need to drop more than 1 block to keep this even
        int iNumDrop;
        if ( iPlyrsLeft <= 0 )
            iNumDrop = 0;
        else
        {
            iNumDrop = ( iOceansLeft - iIslandPlayersLeft * 2 ) / iPlyrsLeft;
            iNumDrop = __min( iNumDrop, ( iNumBlks - iInd ) - iPlyrsLeft );
        }

        while ( ( iInd < iNumBlks ) && ( piBlks[iInd] == 0 ) && ( iNumDrop-- > 0 ) )
        {
            int  iTestOcean  = piBlks[IndLeft( iInd, iSide )];
            int  iTestIsland = piBlks[IndNext( iInd, iSide )];
            BOOL bOcean      = FALSE;
            if ( iTestOcean > 0 )
                if ( theGame.GetPlayerByPlyr( iTestOcean )->m_InitData.GetSupplies( CRaceDef::island ) )
                    bOcean = TRUE;
            if ( iTestIsland > 0 )
                if ( theGame.GetPlayerByPlyr( iTestIsland )->m_InitData.GetSupplies( CRaceDef::island ) )
                    bOcean = TRUE;
            if ( ( !bOcean ) && ( pos != NULL ) )
            {
                POSITION _pos  = pos;
                CPlayer* _pPlr = theGame.GetAll( ).GetNext( _pos );
                if ( ( _pPlr ) && ( _pPlr->m_InitData.GetSupplies( CRaceDef::island ) ) )
                    bOcean = TRUE;
            }

            ASSERT( iInd < iNumBlks );
            if ( bOcean )
                piBlks[iInd] = -1;
            else
            {
                // pick - 1, -2, -3
                int iRand    = ( MyRand( ) >> 12 ) & 0x03;
                piBlks[iInd] = -( iRand + 1 );
            }
            iInd++;
            iOceansLeft--;

            // skip to the next blk
            while ( piBlks[iInd] != 0 )
            {
                if ( iInd >= iNumBlks - iPlyrsLeft )
                    break;
                iInd++;
            }
            if ( piBlks[iInd] != 0 )
                break;
        }
    }  // end of setting piBlks[]

    // set remaining blocks (if any) to ocean (-1), desert (-2), swamp (-3), or plains (-4)
    while ( iInd < iNumBlks )
    {
        if ( piBlks[iInd] == 0 )
        {
            int iRtn = ( MyRand( ) >> 11 ) & 0x03;
            if ( iRtn == 0 )
                iRtn = ( ( MyRand( ) >> 10 ) & 0x01 ) + 2;
            ASSERT( iInd < iNumBlks );
            piBlks[iInd] = -iRtn;
        }
        iInd++;
    }

    // OK, we now set up each blk by initializing certain points in their
    // grid before calling InitSquare. We call InitSquare for each blk
    // seperately so we can set the variation in altitude on a per blk basis.

    const int NUM_OCEAN             = 17;
    const int ocean[NUM_OCEAN][3]   = { 0,  0,  1,  32, 0,  1,  64, 0,  1,  16, 16, 10, 32, 16, 10, 48, 16,
                                      10, 0,  32, 1,  16, 32, 10, 32, 32, 30, 48, 32, 10, 64, 32, 1,  16,
                                      48, 10, 32, 48, 10, 48, 48, 10, 0,  64, 1,  32, 64, 1,  64, 64, 1 };
    const int NUM_ISLAND            = 13;
    const int island[NUM_ISLAND][3] = { 0,  0,  1,  32, 0,  1,  64, 0,  1,  16, 16, 50, 48, 16, 50, 0, 32, 1,  32, 32,
                                        60, 64, 32, 1,  16, 48, 50, 48, 48, 50, 0,  64, 1,  32, 64, 1, 64, 64, 1 };
    const int NUM_SWAMP             = 9;
    const int swamp[NUM_SWAMP][3]   = { 0,  0,  20, 64, 0,  20, 32, 16, 20, 16, 32, 20, 32, 32,
                                      30, 48, 32, 20, 32, 48, 20, 0,  64, 20, 64, 64, 20 };
    const int NUM_LAND              = 9;
    const int land[NUM_LAND][3]     = { 0,  0,  30, 64, 0,  30, 32, 16, 40, 16, 32, 40, 32, 32,
                                    50, 48, 32, 40, 32, 48, 40, 0,  64, 30, 64, 64, 30 };

    int _x = 0, _y = 0;
    for ( iInd = 0; iInd < iNumBlks; iInd++ )
    {
        theApp.m_pCreateGame->GetDlgStatus( )->SetPer( PER_WORLD_ASSIGN + ( iInd * PER_NUM_WORLD_ASSIGN ) / iNumBlks );

        switch ( piBlks[iInd] )
        {
        // ocean
        case -1: {
            for ( int iOn = 0; iOn < NUM_OCEAN; iOn++ )
                ( GetHex( CHexCoord( _x * iSideSize + ( ocean[iOn][0] * iSideSize ) / 64,
                                     _y * iSideSize + ( ocean[iOn][1] * iSideSize ) / 64 ) ) )
                    ->Init( ConvertAlt( ocean[iOn][2] / 2 + RandNum( ocean[iOn][2] ), iSideSize ) );
            break;
        }

        // desert
        // swamp
        case -2:
        case -3: {
            int iAlt = RandNum( 20 ) - 5;  // force different avg altitudes
            for ( int iOn = 0; iOn < NUM_SWAMP; iOn++ )
                ( GetHex( CHexCoord( _x * iSideSize + ( swamp[iOn][0] * iSideSize ) / 64,
                                     _y * iSideSize + ( swamp[iOn][1] * iSideSize ) / 64 ) ) )
                    ->Init( ConvertAlt( iAlt + swamp[iOn][2] / 2 + RandNum( swamp[iOn][2] ), iSideSize ) );
            break;
        }

        // land
        case -4: {
            int iAlt = RandNum( 20 ) - 5;  // force different avg altitudes
            for ( int iOn = 0; iOn < NUM_LAND; iOn++ )
                ( GetHex( CHexCoord( _x * iSideSize + ( land[iOn][0] * iSideSize ) / 64,
                                     _y * iSideSize + ( land[iOn][1] * iSideSize ) / 64 ) ) )
                    ->Init( ConvertAlt( iAlt + land[iOn][2] / 2 + RandNum( land[iOn][2] ), iSideSize ) );
            break;
        }

        default: {
            CPlayer* pPlyr = theGame.GetPlayerByPlyr( piBlks[iInd] );
            if ( pPlyr->m_InitData.GetSupplies( CRaceDef::island ) )
                for ( int iOn = 0; iOn < NUM_ISLAND; iOn++ )
                {
                    int iAlt = island[iOn][2];
                    ( GetHex( CHexCoord( _x * iSideSize + ( island[iOn][0] * iSideSize ) / 64,
                                         _y * iSideSize + ( island[iOn][1] * iSideSize ) / 64 ) ) )
                        ->Init( ConvertAlt( iAlt / 2 + RandNum( iAlt ), iSideSize ) );
                }
            else
                for ( int iOn = 0; iOn < NUM_LAND; iOn++ )
                {
                    int iAlt = land[iOn][2];
                    ( GetHex( CHexCoord( _x * iSideSize + ( land[iOn][0] * iSideSize ) / 64,
                                         _y * iSideSize + ( land[iOn][1] * iSideSize ) / 64 ) ) )
                        ->Init( ConvertAlt( iAlt / 2 + RandNum( iAlt ), iSideSize ) );
                }
            break;
        }
        }

        _y++;
        if ( _y >= iSide )
        {
            _x++;
            _y = 0;
        }
    }

    // set the altitude for each block
    theApp.m_pCreateGame->GetDlgStatus( )->SetMsg( IDS_INIT_MAP );
    _x = _y = 0;
    for ( iInd = 0; iInd < iNumBlks; iInd++ )
    {
        theApp.m_pCreateGame->GetDlgStatus( )->SetPer( PER_WORLD_ALT + ( iInd * PER_NUM_WORLD_ALT ) / iNumBlks );

        // we init this block
        int iAlt1, iAlt2, iAlt3, iAlt4;
        iAlt1 = ( GetHex( CHexCoord( _x * iSideSize, _y * iSideSize ) ) )->GetAlt( );
        iAlt2 = ( GetHex( CHexCoord( ( _x + 1 ) * iSideSize, _y * iSideSize ) ) )->GetAlt( );
        iAlt3 = ( GetHex( CHexCoord( _x * iSideSize, ( _y + 1 ) * iSideSize ) ) )->GetAlt( );
        iAlt4 = ( GetHex( CHexCoord( ( _x + 1 ) * iSideSize, ( _y + 1 ) * iSideSize ) ) )->GetAlt( );
        InitSquare( _x * iSideSize, _y * iSideSize, ( _x + 1 ) * iSideSize, ( _y + 1 ) * iSideSize, iAlt1, iAlt2, iAlt3,
                    iAlt4 );

        // set the terrain type
        for ( int x = _x * iSideSize; x < ( _x + 1 ) * iSideSize; x++ )
            for ( int y = _y * iSideSize; y < ( _y + 1 ) * iSideSize; y++ )
            {
                CHex* pHexOn = GetHex( CHexCoord( x, y ) );
                int   iAlt   = pHexOn->GetAlt( );

                if ( iAlt <= CHex::sea_level && GetHex( CHexCoord( x + 1, y ) )->GetAlt( ) <= CHex::sea_level &&
                     GetHex( CHexCoord( x + 1, y + 1 ) )->GetAlt( ) <= CHex::sea_level &&
                     GetHex( CHexCoord( x, y + 1 ) )->GetAlt( ) <= CHex::sea_level )
                    pHexOn->SetType( CHex::ocean );
                else

                    switch ( piBlks[iInd] )
                    {
                    case -1: {  // ocean
                        int iRand = RandNum( 8 );
                        if ( iAlt < ConvertAlt( 38 + iRand, iSideSize ) )
                            pHexOn->SetType( CHex::plain );
                        else if ( iAlt < ConvertAlt( 42 + iRand, iSideSize ) )
                            pHexOn->SetType( CHex::rough );
                        else if ( iAlt < ConvertAlt( 46 + iRand, iSideSize ) )
                            pHexOn->SetType( CHex::hill );
                        else
                            pHexOn->SetType( CHex::mountain );
                        break;
                    }
                    case -2: {  // dessert
                        int iRand = RandNum( 8 );
                        if ( iAlt < ConvertAlt( 26 + iRand, iSideSize ) )
                            pHexOn->SetType( CHex::desert );
                        else if ( iAlt < ConvertAlt( 36 + iRand, iSideSize ) )
                            pHexOn->SetType( CHex::rough );
                        else if ( iAlt < ConvertAlt( 56 + iRand, iSideSize ) )
                            pHexOn->SetType( CHex::hill );
                        else
                            pHexOn->SetType( CHex::mountain );
                        break;
                    }
                    case -3: {  // swamp
                        int iRand = RandNum( 8 );
                        if ( iAlt < ConvertAlt( 26 + iRand, iSideSize ) )
                            pHexOn->SetType( CHex::swamp );
                        else if ( iAlt < ConvertAlt( 36 + iRand, iSideSize ) )
                            pHexOn->SetType( CHex::rough );
                        else if ( iAlt < ConvertAlt( 56 + iRand, iSideSize ) )
                            pHexOn->SetType( CHex::hill );
                        else
                            pHexOn->SetType( CHex::mountain );
                        break;
                    }
                    default: {  // land
                        int iRand = RandNum( 8 );
                        if ( iAlt < ConvertAlt( 56 + iRand, iSideSize ) )
                            pHexOn->SetType( CHex::plain );
                        else if ( iAlt < ConvertAlt( 64 + iRand, iSideSize ) )
                            pHexOn->SetType( CHex::hill );
                        else
                            pHexOn->SetType( CHex::mountain );
                        break;
                    }
                    }
            }

        // merge in other terrain types
        int       iOff[4][2] = { 12, 12, 44, 12, 12, 44, 44, 44 };
        const int iTry1[]    = { CHex::rough, CHex::hill, CHex::forest, 0 };
        const int iTry2[]    = { CHex::rough, CHex::rough,  CHex::rough, CHex::hill,   CHex::hill,
                              CHex::hill,  CHex::desert, CHex::swamp, CHex::forest, 0 };
        switch ( piBlks[iInd] )
        {
        case -1: {  // ocean
            const int iTry[] = { CHex::rough, CHex::plain, CHex::hill, CHex::forest };
            MakeTerrain( _x * iSideSize + 4 + RandNum( iSideSize - 8 ), _y * iSideSize + 4 + RandNum( iSideSize - 8 ),
                         iTry[RandNum( 3 )], iSideSize );
            MakeTerrain( _x * iSideSize + 4 + RandNum( iSideSize - 8 ), _y * iSideSize + 4 + RandNum( iSideSize - 8 ),
                         iTry[RandNum( 3 )], iSideSize );
            MakeMineral( _x * iSideSize + iSideSize / 2, _y * iSideSize + 4 + iSideSize / 2, CMaterialTypes::copper,
                         iSideSize / 4 );
            MakeMineral( _x * iSideSize + iSideSize / 2, _y * iSideSize + 4 + iSideSize / 2, CMaterialTypes::oil,
                         iSideSize / 4 );
            break;
        }

        case -2:    // desert
        case -3: {  // swamp
            int x = _x * iSideSize + 8 + RandNum( iSideSize - 16 );
            int y = _y * iSideSize + 8 + RandNum( iSideSize - 16 );
            MakeTerrain( x, y, CHex::plain, iSideSize );
            x = _x * iSideSize + 8 + RandNum( iSideSize - 16 );
            y = _y * iSideSize + 8 + RandNum( iSideSize - 16 );
            MakeTerrain( x, y, CHex::rough, iSideSize );

            MakeMineral( _x * iSideSize + 4 + RandNum( iSideSize - 8 ), _y * iSideSize + 4 + RandNum( iSideSize - 8 ),
                         CMaterialTypes::oil, iSideSize );
            MakeMineral( _x * iSideSize + 4 + RandNum( iSideSize - 8 ), _y * iSideSize + 4 + RandNum( iSideSize - 8 ),
                         CMaterialTypes::copper, iSideSize / 4 );
            MakeMineral( _x * iSideSize + 4 + RandNum( iSideSize - 8 ), _y * iSideSize + 4 + RandNum( iSideSize - 8 ),
                         CMaterialTypes::oil, iSideSize );
            MakeMineral( _x * iSideSize + 4 + RandNum( iSideSize - 8 ), _y * iSideSize + 4 + RandNum( iSideSize - 8 ),
                         CMaterialTypes::copper, iSideSize / 4 );
            break;
        }

        case -4: {  // unoccupied plains
            int xDrop = _x * iSideSize + 8 + RandNum( iSideSize - 16 );
            int yDrop = _y * iSideSize + 8 + RandNum( iSideSize - 16 );
            MakeTerrain( xDrop, yDrop, CHex::forest, iSideSize * 2 );
            MakeMineral( _x * iSideSize + 4 + RandNum( iSideSize - 8 ), _y * iSideSize + 4 + RandNum( iSideSize - 8 ),
                         CMaterialTypes::coal, iSideSize );
            MakeMineral( _x * iSideSize + 4 + RandNum( iSideSize - 8 ), _y * iSideSize + 4 + RandNum( iSideSize - 8 ),
                         CMaterialTypes::iron, iSideSize / 4 );
            xDrop = _x * iSideSize + 8 + RandNum( iSideSize - 16 );
            yDrop = _y * iSideSize + 8 + RandNum( iSideSize - 16 );
            MakeTerrain( xDrop, yDrop, iTry2[RandNum( 8 )], iSideSize );
            break;
        }

        // we randomly put stuff in 3 of the 4 sub-blocks. 1 is always forest
        default: {
            int  iForest = 0;
            BOOL bSwamp  = FALSE;
            BOOL bRough  = FALSE;
            for ( int iTry = 0; iTry < 3; iTry++ )
            {
                int iInd = RandNum( 3 - iTry );
                int iOn  = 0;
                while ( ( iInd > 0 ) || ( iOff[iOn][0] == 0 ) )
                {
                    if ( iOff[iOn][0] != 0 )
                        iInd--;
                    iOn++;
                }
                ASSERT( iOn < 4 );

                // get the terrain type
                int iTyp = CHex::plain;
                switch ( iTry )
                {
                case 0:
                    iTyp = iTry1[RandNum( 3 )];
                    break;
                case 1:
                    iTyp = iTry2[RandNum( 8 )];
                    break;
                case 2:
                    if ( iForest < 2 )
                        iTyp = CHex::forest;
                    else if ( !bSwamp )
                        iTyp = iTry2[RandNum( 8 )];
                    else
                        iTyp = iTry1[RandNum( 3 )];
                    break;
                }

                if ( iTyp != 0 )
                {
                    if ( iTyp == CHex::forest )
                        iForest++;
                    if ( iTyp == CHex::rough )
                    {
                        if ( bRough )
                            iTyp = CHex::forest;
                        else
                            bRough = TRUE;
                    }

                    int xDrop = ( ( iOff[iOn][0] + RandNum( 8 ) ) * iSideSize ) / 64 + _x * iSideSize;
                    int yDrop = ( ( iOff[iOn][1] + RandNum( 8 ) ) * iSideSize ) / 64 + _y * iSideSize;
                    MakeTerrain( xDrop, yDrop, iTyp, iTyp == CHex::forest ? iSideSize * 2 : iSideSize );
                    if ( ( iTyp == CHex::swamp ) || ( iTyp == CHex::desert ) )
                    {
                        bSwamp = TRUE;
                        MakeMineral( xDrop, yDrop, CMaterialTypes::oil, 4 );
                    }
                    xDrop = ( ( iOff[iOn][0] + RandNum( 8 ) ) * iSideSize ) / 64 + _x * iSideSize;
                    yDrop = ( ( iOff[iOn][1] + RandNum( 8 ) ) * iSideSize ) / 64 + _y * iSideSize;
                    MakeTerrain( xDrop, yDrop, iTyp, iSideSize / 2 );
                    if ( ( iTyp == CHex::swamp ) || ( iTyp == CHex::desert ) )
                    {
                        MakeMineral( xDrop, yDrop, CMaterialTypes::copper, 4 );
                        MakeMineral( xDrop, yDrop, CMaterialTypes::oil, 4 );
                    }
                    iOff[iOn][0] = 0;
                }
            }

            // is this an AI or HP?
            CPlayer* pPlyr = theGame.GetPlayerByPlyr( piBlks[iInd] );
            int      iMax;
            if ( ( pPlyr == NULL ) || ( !pPlyr->IsAI( ) ) )
                iMax = iSideSize;
            else
                iMax = iSideSize / 2 + ( theGame.m_iAi * iSideSize ) / 2;
            iMax += iMax / 2;

            if ( DepositMinerals( _x, _y, CMaterialTypes::copper, iMax / 4 ) < iMax / 8 )
                DepositMinerals( _x, _y, CMaterialTypes::copper, iMax / 8 );
            if ( DepositMinerals( _x, _y, CMaterialTypes::oil, iMax / 2 ) < iMax / 4 )
                DepositMinerals( _x, _y, CMaterialTypes::oil, iMax / 4 );
            if ( DepositMinerals( _x, _y, CMaterialTypes::iron, iMax / 2 ) < iMax / 4 )
                DepositMinerals( _x, _y, CMaterialTypes::iron, iMax / 4 );
            if ( DepositMinerals( _x, _y, CMaterialTypes::coal, iMax / 2 ) < iMax / 4 )
                DepositMinerals( _x, _y, CMaterialTypes::coal, iMax / 4 );

            // let's give the AI some real close minerals
            if ( ( pPlyr != NULL ) && pPlyr->IsAI( ) )
            {
                int       iNum   = ( ( theGame.m_iAi + 1 ) * m_iSideSize ) / 20;
                const int xBase  = _x * m_iSideSize;
                const int yBase  = _y * m_iSideSize;
                const int iLeft  = m_iSideSize / 2 - m_iSideSize / 8;
                const int iRight = m_iSideSize / 2 + m_iSideSize / 8;
                MakeMineral( xBase + iLeft, yBase + iLeft, CMaterialTypes::copper, iNum );
                MakeMineral( xBase + iLeft, yBase + iRight, CMaterialTypes::oil, iNum * 3 );
                MakeMineral( xBase + iRight, yBase + iLeft, CMaterialTypes::iron, iNum * 4 );
                MakeMineral( xBase + iRight, yBase + iRight, CMaterialTypes::coal, iNum * 2 );
            }

            break;
        }
        }

        _y++;
        if ( _y >= iSide )
        {
            _x++;
            _y = 0;
        }
    }

    // at corners of blocks we may place a mountain
    for ( iInd = 0; iInd < iNumBlks; iInd++ )
    {
        CHex* pHexOn = GetHex( CHexCoord( _x, _y ) );
        if ( ( ( pHexOn->GetType( ) != CHex::ocean ) && ( pHexOn->GetAlt( ) < ConvertAlt( 70, iSideSize ) ) ) &&
             ( ( iInd == 0 ) || ( MyRand( ) & 0x0100 ) ) )
        {
            theApp.m_pCreateGame->GetDlgStatus( )->SetPer( PER_WORLD_WATER +
                                                           ( iInd * PER_NUM_WORLD_WATER ) / iNumBlks );

            _x = ( iInd / iSide ) * iSideSize - 4 + RandNum( 8 );
            _y = ( iInd % iSide ) * iSideSize - 4 + RandNum( 8 );

            CHex* pHexOn = GetHex( CHexCoord( _x, _y ) );
            if ( ( pHexOn->GetType( ) != CHex::ocean ) && ( pHexOn->GetAlt( ) < 70 ) && ( MyRand( ) & 0x0100 ) )
            {
                int iAlt = __min( 72, pHexOn->GetAlt( ) + 36 ) + RandNum( 8 );
                pHexOn->SetAlt( ConvertAlt( iAlt, iSideSize ) );
                if ( pHexOn->GetAlt( ) < ConvertAlt( 50, iSideSize ) )
                    pHexOn->SetType( CHex::hill );
                else
                    pHexOn->SetType( CHex::mountain );

                int  iWid  = 1;
                BOOL bDone = FALSE;
                do
                {
                    bDone = TRUE;
                    for ( int y = _y - iWid + 1; y < _y + iWid; y++ )
                    {
                        if ( MakePeak( _x - iWid + 1, y, _x - iWid, y, iSideSize, y != 0 ) )
                            bDone = FALSE;
                        if ( MakePeak( _x + iWid - 1, y, _x + iWid, y, iSideSize, y != 0 ) )
                            bDone = FALSE;
                    }
                    for ( int x = _x - iWid; x < _x + iWid + 1; x++ )
                    {
                        if ( MakePeak( x, _y - iWid + 1, x, _y - iWid, iSideSize, x != 0 ) )
                            bDone = FALSE;
                        if ( MakePeak( x, _y + iWid - 1, x, _y + iWid, iSideSize, x != 0 ) )
                            bDone = FALSE;
                    }
                    iWid += 1;
                } while ( !bDone );

#ifdef BUGBUG
                // now we smooth it out
                iWid                = ( iWid + 2 ) & ~0x01;
                iWid                = __max( iWid, 4 );
                const int aiShft[3] = { 0, -1, 1 };
                for ( int iPass = 0; iPass <= 2; iPass++ )
                {
                    for ( int xMul = -1; xMul <= 1; xMul++ )
                        for ( int yMul = -1; yMul <= 1; yMul++ )
                        {
                            int xMin  = _x + iWid * xMul + aiShft[iPass];
                            int yMin  = _y + iWid * yMul + aiShft[iPass];
                            int xMax  = _x + iWid * ( xMul + 1 ) + aiShft[iPass];
                            int yMax  = _y + iWid * ( yMul + 1 ) + aiShft[iPass];
                            int iAlt1 = GetHex( xMin, yMin )->GetAlt( );
                            int iAlt2 = GetHex( xMax, yMin )->GetAlt( );
                            int iAlt3 = GetHex( xMin, yMax )->GetAlt( );
                            int iAlt4 = GetHex( xMax, yMax )->GetAlt( );
                            InitSquarePass2( xMin, yMin, xMax, yMax, iAlt1, iAlt2, iAlt3, iAlt4 );
                        }
                    iWid += 2;
                }
#endif

                // its a mountain - load it up with coal & iron
                int iDif = iWid / 2;
                MakeMineral( _x - iDif, _y - iDif, CMaterialTypes::coal, iWid );
                MakeMineral( _x + iDif, _y + iDif, CMaterialTypes::iron, iWid );
            }
        }
    }

    // we now walk the borders between blocks averaging types across the line
    // we push up to iSideSize/8 hexes in, and build a new border
    _x = _y = 0;
    for ( iInd = 0; iInd < iNumBlks; iInd++ )
    {
        theApp.m_pCreateGame->GetDlgStatus( )->SetPer( PER_WORLD_AVG + ( iInd * PER_NUM_WORLD_AVG ) / iNumBlks );

        // top
        int y    = _y * iSideSize;
        int yMax = y + iSideSize / 4;
        int yMin = y - iSideSize / 4;
        int yOn  = y;
        for ( int x = _x * iSideSize; x < ( _x + 1 ) * iSideSize; x++ )
        {
            // we drop the yMin/Max as we get close to bring it back together
            int iLeft = ( _x + 1 ) * iSideSize - x;
            if ( iLeft < iSideSize / 4 )
            {
                yMax = y + iLeft;
                yMin = y - iLeft;
            }

            yOn += RandNum( 2 ) - 1;
            yOn = __min( yOn, yMax );
            yOn = __max( yOn, yMin );
            if ( yOn > y )
            {
                CHex* pHexOn = GetHex( CHexCoord( x, y - 2 ) );
                if ( pHexOn->IsWater( ) )
                    continue;
                int iTyp = pHexOn->GetType( );
                for ( int _y = y; _y <= yOn; _y++ )
                {
                    CHex* pHexSet = GetHex( CHexCoord( x, _y ) );
                    if ( !pHexSet->IsWater( ) )
                    {
                        if ( MyRand( ) & 0x0100 )
                            pHexSet->SetType( iTyp );
                        else
                        {
                            CHex* pHex2 = GetHex( CHexCoord( x - 1, _y ) );
                            if ( !pHex2->IsWater( ) )
                                pHexSet->SetType( pHex2->GetType( ) );
                        }
                    }
                }
            }
            else if ( yOn < y )
            {
                CHex* pHexOn = GetHex( CHexCoord( x, y + 2 ) );
                if ( pHexOn->IsWater( ) )
                    continue;
                int iTyp = pHexOn->GetType( );
                for ( int _y = y; _y >= yOn; _y-- )
                {
                    CHex* pHexSet = GetHex( CHexCoord( x, _y ) );
                    if ( !pHexSet->IsWater( ) )
                    {
                        if ( MyRand( ) & 0x0100 )
                            pHexSet->SetType( iTyp );
                        else
                        {
                            CHex* pHex2 = GetHex( CHexCoord( x - 1, _y ) );
                            if ( !pHex2->IsWater( ) )
                                pHexSet->SetType( pHex2->GetType( ) );
                        }
                    }
                }
            }
        }

        // left side
        int x    = _x * iSideSize;
        int xMax = x + iSideSize / 4;
        int xMin = x - iSideSize / 4;
        int xOn  = x;
        for ( y = _y * iSideSize; y < ( _y + 1 ) * iSideSize; y++ )
        {
            // we drop the yMin/Max as we get close to bring it back together
            int iLeft = ( _y + 1 ) * iSideSize - y;
            if ( iLeft < iSideSize / 4 )
            {
                xMax = x + iLeft;
                xMin = x - iLeft;
            }

            xOn += RandNum( 2 ) - 1;
            xOn = __min( xOn, xMax );
            xOn = __max( xOn, xMin );
            if ( xOn > x )
            {
                CHex* pHexOn = GetHex( CHexCoord( x - 2, y ) );
                if ( pHexOn->IsWater( ) )
                    continue;
                int iTyp = pHexOn->GetType( );
                for ( int _x = x; _x <= xOn; _x++ )
                {
                    CHex* pHexSet = GetHex( CHexCoord( _x, y ) );
                    if ( !pHexSet->IsWater( ) )
                    {
                        if ( MyRand( ) & 0x0100 )
                            pHexSet->SetType( iTyp );
                        else
                        {
                            CHex* pHex2 = GetHex( CHexCoord( _x, y - 1 ) );
                            if ( !pHex2->IsWater( ) )
                                pHexSet->SetType( pHex2->GetType( ) );
                        }
                    }
                }
            }
            else if ( xOn < x )
            {
                CHex* pHexOn = GetHex( CHexCoord( x + 2, y ) );
                if ( pHexOn->IsWater( ) )
                    continue;
                int iTyp = pHexOn->GetType( );
                for ( int _x = x; _x >= xOn; _x-- )
                {
                    CHex* pHexSet = GetHex( CHexCoord( _x, y ) );
                    if ( !pHexSet->IsWater( ) )
                    {
                        if ( MyRand( ) & 0x0100 )
                            pHexSet->SetType( iTyp );
                        else
                        {
                            CHex* pHex2 = GetHex( CHexCoord( _x, y - 1 ) );
                            if ( !pHex2->IsWater( ) )
                                pHexSet->SetType( pHex2->GetType( ) );
                        }
                    }
                }
            }
        }

        _y++;
        if ( _y >= iSide )
        {
            _x++;
            _y = 0;
        }
    }

    // we re-smooth the entire world
    int iAlt = GetHex( 0, 0 )->GetAlt( );
    InitSquarePass2( 0, 0, m_eX, m_eY, iAlt, iAlt, iAlt, iAlt );

    // check for too large an alt increase
    theApp.BaseYield( );
    CheckAlt( );

    // put rivers down the peaks
    _x = 8;
    _y = 0;
    for ( iInd = 0; iInd < iNumBlks; iInd++ )
    {
        CHex* pHexOn = GetHex( CHexCoord( _x, _y ) );
        if ( ( pHexOn->GetType( ) != CHex::ocean ) && ( pHexOn->GetType( ) != CHex::river ) &&
             ( piBlks[IndNext( iInd, iSide )] > 0 ) )
        {
            // do a river?
            int iRiver = ( MyRand( ) >> 12 );
            if ( ( iScenario ) || ( iRiver & 0x01 ) )
            {
                BOOL bFound = FALSE;
                MakeRiver( _x + RandNum( 2 ) - 1, _y + 2 + RandNum( 4 ), bFound );
            }
            if ( ( iScenario ) || ( iRiver & 0x02 ) )
            {
                BOOL bFound = FALSE;
                MakeRiver( _x - 2 - RandNum( 4 ), _y + RandNum( 2 ) - 1, bFound );
            }
            if ( ( iScenario ) || ( iRiver & 0x04 ) )
            {
                BOOL bFound = FALSE;
                MakeRiver( _x + 2 + RandNum( 4 ), _y + RandNum( 2 ) - 1, bFound );
            }
            if ( ( iScenario ) || ( iRiver & 0x08 ) )
            {
                BOOL bFound = FALSE;
                MakeRiver( _x + RandNum( 2 ) - 1, _y - 2 - RandNum( 4 ), bFound );
            }
        }  // if ! water

        _y += iSideSize;
        if ( _y >= m_eY )
        {
            _x += iSideSize;
            _y = 0;
        }
    }

    theApp.m_pCreateGame->GetDlgStatus( )->SetMsg( IDS_CHECK_MAP );

    delete[] piBlks;

    CheckOcean( );

    // check for land & water crossing on an X
    // we have x,y in the dec/inc so we are comparing to already changed hexes
    for ( int x = m_eX - iSideSize + 8; x > -iSideSize; x-- )
    {
        theApp.m_pCreateGame->GetDlgStatus( )->SetPer(
            PER_WORLD_CHECK + ( ( ( m_eX - iSideSize + 8 ) - x ) * PER_NUM_WORLD_CHECK ) / ( m_eX + 8 ) );

        for ( int y = -iSideSize; y < m_eY - iSideSize + 8; y++ )
        {
            CHex* pHexOn    = GetHex( CHexCoord( x, y ) );
            CHex* pHexBelow = GetHex( CHexCoord( x, y + 1 ) );

            if ( pHexOn->IsWater( ) != pHexBelow->IsWater( ) )
            {
                CHex* pHexLeft = GetHex( CHexCoord( x - 1, y ) );
                CHex* pHexLL   = GetHex( CHexCoord( x - 1, y + 1 ) );

                // check from us to LL
                if ( ( pHexOn->IsWater( ) != pHexLeft->IsWater( ) ) && ( pHexOn->IsWater( ) == pHexLL->IsWater( ) ) )
                {
                    if ( pHexOn->IsWater( ) )
                    {
                        pHexOn->SetType( pHexBelow->GetType( ) );
                        pHexOn->SetAlt( __max( CHex::sea_level + 1, pHexOn->GetAlt( ) ) );
                    }
                    else
                    {
                        pHexBelow->SetType( pHexOn->GetType( ) );
                        pHexBelow->SetAlt( __max( CHex::sea_level + 1, pHexBelow->GetAlt( ) ) );
                    }
                }

                CHex* pHexRight = GetHex( CHexCoord( x + 1, y ) );
                CHex* pHexLR    = GetHex( CHexCoord( x + 1, y + 1 ) );

                // check from us to LR
                if ( ( pHexOn->IsWater( ) != pHexRight->IsWater( ) ) && ( pHexOn->IsWater( ) == pHexLR->IsWater( ) ) )
                {
                    if ( pHexOn->IsWater( ) )
                    {
                        pHexOn->SetType( pHexBelow->GetType( ) );
                        pHexOn->SetAlt( __max( CHex::sea_level + 1, pHexOn->GetAlt( ) ) );
                    }
                    else
                    {
                        pHexBelow->SetType( pHexOn->GetType( ) );
                        pHexBelow->SetAlt( __max( CHex::sea_level + 1, pHexBelow->GetAlt( ) ) );
                    }
                }
            }
        }
    }

    // we assign hill & mountain tiles based on the slope
    for ( int x = 0; x < m_eX; x++ )
        for ( int y = 0; y < m_eY; y++ )
        {
            CHex* pHexOn = GetHex( CHexCoord( x, y ) );
            if ( ( !pHexOn->IsWater( ) ) && ( pHexOn->GetType( ) != CHex::mountain ) )
            {
                CHex* pHexUR    = GetHex( CHexCoord( x + 1, y - 1 ) );
                CHex* pHexTop   = GetHex( CHexCoord( x, y - 1 ) );
                CHex* pHexRight = GetHex( CHexCoord( x + 1, y ) );

                int iSlope  = abs( pHexOn->GetAlt( ) - pHexTop->GetAlt( ) );
                int iSlope2 = abs( pHexOn->GetAlt( ) - pHexRight->GetAlt( ) );
                iSlope      = __max( iSlope, iSlope2 );
                iSlope2     = abs( pHexOn->GetAlt( ) - pHexUR->GetAlt( ) );
                iSlope      = __max( iSlope, iSlope2 );
                if ( iSlope > 15 )
                    pHexOn->SetType( CHex::mountain );
                else if ( iSlope > 8 )
                    pHexOn->SetType( CHex::hill );
            }
        }

    // we now eliminate all single tiles, fingers, etc.
    //   call before assigning tiles, adding coastlines
    theApp.BaseYield( );
    EliminateSingles( );

    // assign tiles based on adjacent altitudes, terrain type
    theApp.m_pCreateGame->GetDlgStatus( )->SetMsg( IDS_ASSIGN_TILES );
    int   lPerStep = ( m_eX * m_eY ) / PER_NUM_WORLD_TILES;
    int   lPerOn   = 0;
    int   iPer     = PER_WORLD_TILES;
    CHex* pHex     = m_pHex;

    theApp.BaseYield( );
    CheckOcean( );

#ifdef BUGBUG
    for ( int lOn = 0; lOn < lTotal; lOn++ )
    {
        lPerOn++;
        if ( lPerOn >= lPerStep )
        {
            lPerOn = 0;
            theApp.m_pCreateGame->GetDlgStatus( )->SetPer( iPer++ );
        }

        pHex->InitType( );
        pHex++;
    }
#endif

    // assign trees based on neighbors
    CHexCoord _hex( 0, 0 );
    pHex = m_pHex;
    for ( int lOn = 0; lOn < lTotal; lOn++ )
    {
        if ( pHex->GetType( ) == CHex::forest )
        {
            int iNext = 0;
            for ( int x = -1; x <= 1; x++ )
                for ( int y = -1; y <= 1; y++ )
                    if ( theMap.GetHex( _hex.X( ) + x, _hex.Y( ) + y )->GetType( ) == CHex::forest )
                    {
                        if ( ( x != 0 ) && ( y != 0 ) )
                            iNext++;
                        else
                            iNext += 2;
                    }

            // we purposely go over to max the most dense art
            int iIndex = ( RandNum( iNext ) * theEffects.TreeCount( ) ) / 7;
            if ( iIndex >= theEffects.TreeCount( ) )
                iIndex -= ( iIndex - theEffects.TreeCount( ) + 1 );
            iIndex = theEffects.TreeCount( ) - iIndex - 1;
            iIndex = __max( iIndex, 0 );
            iIndex = __min( iIndex, theEffects.TreeCount( ) - 1 );
            pHex->SetTree( iIndex );
        }


        pHex++;
        _hex.X( ) += 1;
        if ( _hex.X( ) >= m_eX )
        {
            _hex.X( ) = 0;
            _hex.Y( ) += 1;
        }
    }

    // we now change water tiles on the edge to coastline
    theApp.BaseYield( );
    AddCoastlines( );

    // we now change all small oceans to lakes
    theApp.BaseYield( );
    MakeLakes( );

    // now set m_bVisible to 1 for our landing block (faster than testing above loop)
    if ( theGame.HaveHP( ) )
    {
        CHexCoord hexVis( theGame.GetMe( )->m_hexMapStart.X( ) - iSideSize / 2,
                          theGame.GetMe( )->m_hexMapStart.Y( ) - iSideSize / 2 );
        theMap.EnumHexes( hexVis, iSideSize, iSideSize, fnEnumIncVis, NULL );
    }

    // no minerals under water or on the coast
    pos = theMinerals.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwHex;
        CMinerals* pMn;
        theMinerals.GetNextAssoc( pos, dwHex, pMn );
        CHex* pHex = theMap._GetHex( ( dwHex >> 16 ) & 0xFFFF, dwHex & 0xFFFF );
        if ( ( pHex->IsWater( ) ) || ( pHex->GetType( ) == CHex::coastline ) )
        {
            pHex->NandUnits( CHex::minerals );
            theMinerals.RemoveKey( dwHex );
            delete pMn;
        }
    }

    // we dup the extra line on the bottom
    int   iNum      = m_eX;
    CHex* pHexStart = _GetHex( 0, 0 );
    CHex* pHexDup   = _GetHex( 0, m_eY - 1 ) + m_eX;
    while ( iNum-- ) *pHexDup++ = *pHexStart++;

    theApp.BaseYield( );

    ASSERT_VALID( this );
}

BOOL CGameMap::MakePeak( int xOk, int yOk, int xTest, int yTest, int iSideSize, BOOL bEasy )
{

    CHex* pHexTest = GetHex( CHexCoord( xTest, yTest ) );
    if ( pHexTest->GetAlt( ) <= CHex::sea_level )
        return ( FALSE );

    CHex* pHexOk = GetHex( CHexCoord( xOk, yOk ) );

    if ( pHexOk->GetAdjustStep( ) - pHexTest->GetAdjustStep( ) > 1 )
    {
        int iDiff;
        if ( ( bEasy ) && ( pHexOk->GetAlt( ) >= ConvertAlt( 30, iSideSize ) ) )
            iDiff = RandNum( CHex::map_step / 2 );
        else
            iDiff = RandNum( CHex::map_step / 4 ) + 1;
        if ( ( iDiff != 0 ) && ( ( MyRand( ) & 0x1F ) == 0x1F ) )
            iDiff = -iDiff;

        pHexTest->SetAlt( pHexOk->GetAlt( ) - iDiff );
        int iRand = RandNum( 8 );
        if ( pHexTest->GetAlt( ) < ConvertAlt( 36 + iRand, iSideSize ) )
        {
            if ( MyRand( ) & 0x0011 )
                pHexTest->SetType( CHex::rough );
        }
        else if ( pHexTest->GetAlt( ) < ConvertAlt( 56 + iRand, iSideSize ) )
            pHexTest->SetType( CHex::hill );
        else
            pHexTest->SetType( CHex::mountain );
        return ( TRUE );
    }

    return ( FALSE );
}

void CGameMap::MakeTerrain( int x, int y, int iTyp, int iSideSize )
{

    CHexCoord _hex( x, y );
    int       iNum = ( iSideSize / 2 ) * ( iSideSize / 2 ) / 2;
    iNum           = iNum / 16 + RandNum( iNum );

    while ( iNum-- > 0 )
    {
        CHex* pHexOn = GetHex( _hex );
        if ( pHexOn->GetAlt( ) > CHex::sea_level )
            pHexOn->SetType( iTyp );

        // we want a new hex that isn't the same type
        // every 8th we jump to a new location instead
        int iAvail = 0;
        if ( ( iNum & 0x07 ) != 0 )
            for ( int _x = -1; _x <= 1; _x++ )
                for ( int _y = -1; _y <= 1; _y++ )
                {
                    CHexCoord hexOn( _hex.X( ) + _x, _hex.Y( ) + _y );
                    hexOn.Wrap( );
                    if ( GetHex( hexOn )->GetType( ) != iTyp )
                        iAvail++;
                }

        // if no hex that's not our type - we jump
        if ( iAvail == 0 )
        {
            if ( ( iTyp == CHex::swamp ) || ( iTyp == CHex::desert ) )
            {
                _hex.X( _hex.X( ) - 1 + RandNum( 2 ) );
                _hex.Y( _hex.Y( ) - 1 + RandNum( 2 ) );
            }
            else
            {
                _hex.X( _hex.X( ) - 3 + RandNum( 6 ) );
                _hex.Y( _hex.Y( ) - 3 + RandNum( 6 ) );
            }
        }

        // we take one of the different hexes
        else
        {
            iAvail = RandNum( iAvail - 1 );
            for ( int _x = -1; _x <= 1; _x++ )
                for ( int _y = -1; _y <= 1; _y++ )
                {
                    CHexCoord hexOn( _hex.X( ) + _x, _hex.Y( ) + _y );
                    hexOn.Wrap( );
                    if ( GetHex( hexOn )->GetType( ) != iTyp )
                    {
                        if ( iAvail <= 0 )
                        {
                            _hex = hexOn;
                            goto got_it;
                        }
                        iAvail--;
                    }
                }
        }
    got_it:

        // swamps & dessert are more cohesive
        if ( ( ( iTyp == CHex::swamp ) || ( iTyp == CHex::desert ) ) && ( ( iNum & 0x1F ) == 0 ) )
        {
            int xDif = abs( CHexCoord::Diff( _hex.X( ) - x ) );
            int yDif = abs( CHexCoord::Diff( _hex.Y( ) - y ) );
            if ( xDif > 8 )
                _hex.X( x - xDif / 2 + RandNum( xDif ) );
            if ( yDif > 8 )
                _hex.Y( y - yDif / 2 + RandNum( yDif ) );
        }

        // ok, if we're too far away move back
        if ( abs( CHexCoord::Diff( _hex.X( ) - x ) ) + abs( CHexCoord::Diff( _hex.Y( ) - y ) ) > iSideSize )
            _hex = CHexCoord( x, y );
    }
}

// ok, we now put a sizeable blk at one location and a small one elsewhere
// ALONG THE EDGES
int CGameMap::DepositMinerals( int x, int y, int iTyp, int iNum )
{

    int _x, _y;
    int iSide = ( MyRand( ) & 0x3000 ) >> 12;

    // put on an edge
    switch ( iSide )
    {
    case 0:
        _x = x * m_iSideSize + RandNum( m_iSideSize / 4 );
        _y = y * m_iSideSize + RandNum( m_iSideSize );
        break;
    case 1:
        _x = x * m_iSideSize + RandNum( m_iSideSize );
        _y = y * m_iSideSize + RandNum( m_iSideSize / 4 );
        break;
    case 2:
        _x = ( x + 1 ) * m_iSideSize - RandNum( m_iSideSize / 4 );
        _y = y * m_iSideSize + RandNum( m_iSideSize );
        break;
    default:
        _x = x * m_iSideSize + RandNum( m_iSideSize );
        _y = ( y + 1 ) * m_iSideSize - RandNum( m_iSideSize / 4 );
        break;
    }

    int iRtn = MakeMineral( _x, _y, iTyp, iNum );

    // usually put on oppisate side
    if ( ( MyRand( ) & 0x0300 ) == 0x0300 )
        iSide = ( MyRand( ) & 0x3000 ) >> 12;
    else
        iSide = ( iSide + 2 ) & 0x03;

    switch ( iSide )
    {
    case 0:
        _x = x * m_iSideSize + RandNum( m_iSideSize / 2 );
        _y = y * m_iSideSize + RandNum( m_iSideSize );
        break;
    case 1:
        _x = x * m_iSideSize + RandNum( m_iSideSize );
        _y = y * m_iSideSize + RandNum( m_iSideSize / 2 );
        break;
    case 2:
        _x = ( x + 1 ) * m_iSideSize - RandNum( m_iSideSize / 2 );
        _y = y * m_iSideSize + RandNum( m_iSideSize );
        break;
    default:
        _x = x * m_iSideSize + RandNum( m_iSideSize );
        _y = ( y + 1 ) * m_iSideSize - RandNum( m_iSideSize / 2 );
        break;
    }

    return ( iRtn + MakeMineral( _x, _y, iTyp, __max( m_iSideSize / 8, iNum / 2 ) ) );
}

int CGameMap::MakeMineral( int x, int y, int iTyp, int iSideSize )
{
#ifdef LOGGINGON
   // OutputDebugStringA( "MakeMineral\n" );
#endif


    CHexCoord _hex( x, y );
    _hex.Wrap( );
    iSideSize  = __max( iSideSize, 4 );
    int iNum   = iSideSize / 2 + RandNum( iSideSize );
    int iTotal = 0;

    while ( iNum-- > 0 )
    {
        CHex* pHexOn = _GetHex( _hex );
        if ( ( !pHexOn->IsWater( ) ) && ( !( pHexOn->GetUnits( ) & CHex::minerals ) ) )
        {
            theMinerals.InitHex( _hex, iTyp );
            iTotal++;
        }

        // we want a new hex that isn't the same type
        // every 8th we jump to a new location instead
        int iAvail = 0;
        int iJmp   = 8;
        if ( ( iNum & 0x07 ) != 0 )
        {
            iJmp = 16;
            for ( int _x = -1; _x <= 1; _x++ )
                for ( int _y = -1; _y <= 1; _y++ )
                {
                    CHexCoord hexOn( _hex.X( ) + _x, _hex.Y( ) + _y );
                    hexOn.Wrap( );
                    if ( !( theMap._GetHex( hexOn )->GetUnits( ) & CHex::minerals ) )
                        iAvail++;
                }
        }

        // if no hex that's not our type - we jump
        if ( iAvail == 0 )
        {
            if ( ( iTyp == CMaterialTypes::oil ) || ( iTyp == CMaterialTypes::copper ) )
            {
                _hex.X( _hex.X( ) - iJmp / 4 + RandNum( iJmp / 2 ) );
                _hex.Y( _hex.Y( ) - iJmp / 4 + RandNum( iJmp / 2 ) );
            }
            else
            {
                _hex.X( _hex.X( ) - iJmp / 2 + RandNum( iJmp / 4 ) );
                _hex.Y( _hex.Y( ) - iJmp / 2 + RandNum( iJmp / 4 ) );
            }
            _hex.Wrap( );
        }

        // we take one of the different hexes
        else
        {
            iAvail = RandNum( iAvail );
            for ( int _x = -1; _x <= 1; _x++ )
                for ( int _y = -1; _y <= 1; _y++ )
                {
                    CHexCoord hexOn( _hex.X( ) + _x, _hex.Y( ) + _y );
                    hexOn.Wrap( );
                    if ( !( theMap._GetHex( hexOn )->GetUnits( ) & CHex::minerals ) )
                        iAvail--;
                    if ( iAvail <= 0 )
                    {
                        _hex = hexOn;
                        goto got_it;
                    }
                }
        }
    got_it:

        // oil & copper are more cohesive
        if ( ( ( iTyp == CMaterialTypes::oil ) || ( iTyp == CMaterialTypes::copper ) ) && ( ( iNum & 0x1F ) == 0 ) )
        {
            int xDif = abs( CHexCoord::Diff( _hex.X( ) - x ) );
            int yDif = abs( CHexCoord::Diff( _hex.Y( ) - y ) );
            if ( xDif > m_iSideSize / 8 )
                _hex.X( x - xDif / 2 + RandNum( xDif ) );
            if ( yDif > m_iSideSize / 8 )
                _hex.Y( y - yDif / 2 + RandNum( yDif ) );
            _hex.Wrap( );
        }

        // ok, if we're too far away move back
        if ( abs( CHexCoord::Diff( _hex.X( ) - x ) ) + abs( CHexCoord::Diff( _hex.Y( ) - y ) ) > m_iSideSize / 4 )
        {
            _hex = CHexCoord( x, y );
            _hex.Wrap( );
        }
    }

    return ( iTotal );
}

void CGameMap::MakeRiver( int x, int y, BOOL& bFound )
{
    static int aAlt[4][3] = { 0, -1, 0, -1, 0, 0, +1, 0, 0, 0, +1, 0 };
    int        iInd, iLowest, iLevel, iFound;

    x = theMap.WrapX( x );
    y = theMap.WrapY( y );

    CHex* pHexOn = _GetHex( x, y );
    if ( pHexOn->IsWater( ) )
        return;
    pHexOn->SetType( CHex::river );

    // keep making the lowest neighbor a river till we hit water
    //   or can't go down
    int count = 0;
    while ( count < 32765 ) // this was TRUE, but would take forever sometimes
    {
        count++;
        for ( iInd = 0; iInd < 4; iInd++ )
        {
            CHex* pHex    = GetHex( CHexCoord( x + aAlt[iInd][0], y + aAlt[iInd][1] ) );
            aAlt[iInd][2] = pHex->GetAlt( );
        }

        iLowest = 0;
        for ( iInd = 1; iInd < 4; iInd++ )
            if ( ( aAlt[iInd][2] <= aAlt[iLowest][2] ) )
                iLowest = iInd;
        iLevel = aAlt[iLowest][2];

        // if we can't go down we are done
        if ( ( iLevel > pHexOn->GetAlt( ) ) || ( ( iLevel == pHexOn->GetAlt( ) ) && ( bFound ) ) )
            return;

        // we set all surrounding hexes that are lowest to water
        // if lower AND ! bFound we take a 50% shot at being water
        BOOL bGotOne = FALSE;
        for ( iInd = 0; iInd < 4; iInd++ )
            if ( aAlt[iInd][2] == iLevel )
            {
                CHex* pHex = GetHex( CHexCoord( x + aAlt[iInd][0], y + aAlt[iInd][1] ) );
                if ( !pHex->IsWater( ) )
                {
                    if ( !bGotOne )
                    {
                        if ( pHex->GetType( ) == CHex::ocean )
                            bFound = TRUE;
                        bGotOne = TRUE;
                        pHex->SetType( CHex::river );
                        iFound = iInd;
                    }
                    else if ( ( !bFound ) && ( MyRand( ) & 0x400 ) )
                        MakeRiver( x + aAlt[iInd][0], y + aAlt[iInd][1], bFound );
                }
            }

        // if all lower were water - we're out of here
        if ( bFound || ( !bGotOne ) )
            return;

        // set the new tile
        ASSERT( ( 0 <= iFound ) && ( iFound < 4 ) );
        x      = theMap.WrapX( x + aAlt[iFound][0] );
        y      = theMap.WrapY( y + aAlt[iFound][1] );
        pHexOn = _GetHex( x, y );
    }
}

static int RandDist( int iDist )
{
    /*** gaussian
    int k;
    float value, exponent, gauss;

        k = MyRand () - 16383;
        value = k / 5461.0;
        exponent = - (value * value) / 2.0;
        gauss = 0.15915494 * exp (exponent);

        if (k < 0)
            return (- gauss);
        return (gauss);
    ***/

    iDist /= 2;
    if ( iDist < 1 )
        return ( 0 );

    int iVal = ( ( iDist + 1 ) * ( iDist + 1 ) * MyRand( ) ) / ( RAND_MAX + 1 );
    iVal     = iDist - (int)sqrt( (float)abs( iVal ) );
    ASSERT( ( 0 <= iVal ) && ( iVal <= iDist ) );
    if ( MyRand( ) & 0x1000 )
        return ( iVal );
    return ( -iVal );

    /***
        int iRnd;
        iDist = __max (1, iDist / 4);
        int iRtn = ((int) sqrt ((iRnd = MyRand ()) / (RAND_MAX / 2500))) / 5;
        iRtn += 15;

        return ((iRnd % 80) - 40);
        if (iRnd & 0x1000)
            return (iRtn);
        return (- iRtn);
    */
}

void CGameMap::InitSquare( int x1, int y1, int x2, int y2, int iAlt1, int iAlt2, int iAlt3, int iAlt4 )
{

    if ( ( x1 + 1 >= x2 ) && ( y1 + 1 >= y2 ) )
        return;

    int xDif = x2 - x1;
    int yDif = y2 - y1;
    int _x   = x1 + xDif / 2;
    int _y   = y1 + yDif / 2;
    int _iAlt1, _iAlt2, _iAlt3, _iAlt4, _iAlt5;

    CHex* pHex = GetHex( _x, y1 );
    if ( ( _iAlt1 = pHex->GetAlt( ) ) == 0 )
    {
        int iAltL = GetHex( _x, y1 - yDif / 2 )->GetAlt( );
        int iAltR = GetHex( _x, y1 + yDif / 2 )->GetAlt( );
        if ( ( iAltL != 0 ) && ( iAltR != 0 ) )
            _iAlt1 = ( iAlt1 + iAlt2 + iAltL + iAltR ) / 4 + ( RandDist( xDif ) + RandDist( yDif ) ) / 2;
        else if ( iAltL != 0 )
            _iAlt1 = ( iAlt1 + iAlt2 + iAltL ) / 3 + RandDist( xDif );
        else if ( iAltR != 0 )
            _iAlt1 = ( iAlt1 + iAlt2 + iAltR ) / 3 + RandDist( xDif );
        else
            _iAlt1 = ( iAlt1 + iAlt2 ) / 2 + RandDist( xDif );
        _iAlt1 = _iAlt1 < 0 ? 0 : ( _iAlt1 > 100 ? 100 : _iAlt1 );
        pHex->SetAlt( _iAlt1 );
    }

    pHex = GetHex( x1, _y );
    if ( ( _iAlt2 = pHex->GetAlt( ) ) == 0 )
    {
        int iAltT = GetHex( x1 - xDif / 2, _y )->GetAlt( );
        int iAltB = GetHex( x1 + xDif / 2, _y )->GetAlt( );
        if ( ( iAltT != 0 ) && ( iAltB != 0 ) )
            _iAlt2 = ( iAlt1 + iAlt3 + iAltT + iAltB ) / 4 + ( RandDist( xDif ) + RandDist( yDif ) ) / 2;
        else if ( iAltT != 0 )
            _iAlt2 = ( iAlt1 + iAlt3 + iAltT ) / 3 + RandDist( yDif );
        else if ( iAltB != 0 )
            _iAlt2 = ( iAlt1 + iAlt3 + iAltB ) / 3 + RandDist( yDif );
        else
            _iAlt2 = ( iAlt1 + iAlt3 ) / 2 + RandDist( yDif );
        _iAlt2 = _iAlt2 < 0 ? 0 : ( _iAlt2 > 100 ? 100 : _iAlt2 );
        pHex->SetAlt( _iAlt2 );
    }

    pHex = GetHex( x2, _y );
    if ( ( _iAlt3 = pHex->GetAlt( ) ) == 0 )
    {
        int iAltT = GetHex( x2 - xDif / 2, _y )->GetAlt( );
        int iAltB = GetHex( x2 + xDif / 2, _y )->GetAlt( );
        if ( ( iAltT != 0 ) && ( iAltB != 0 ) )
            _iAlt3 = ( iAlt2 + iAlt4 + iAltT + iAltB ) / 4 + ( RandDist( xDif ) + RandDist( yDif ) ) / 2;
        else if ( iAltT != 0 )
            _iAlt3 = ( iAlt2 + iAlt4 + iAltT ) / 3 + RandDist( yDif );
        else if ( iAltB != 0 )
            _iAlt3 = ( iAlt2 + iAlt4 + iAltB ) / 3 + RandDist( yDif );
        else
            _iAlt3 = ( iAlt2 + iAlt4 ) / 2 + RandDist( yDif );
        _iAlt3 = _iAlt3 < 0 ? 0 : ( _iAlt3 > 100 ? 100 : _iAlt3 );
        pHex->SetAlt( _iAlt3 );
    }

    pHex = GetHex( _x, y2 );
    if ( ( _iAlt4 = pHex->GetAlt( ) ) == 0 )
    {
        int iAltL = GetHex( _x, y2 - yDif / 2 )->GetAlt( );
        int iAltR = GetHex( _x, y2 + yDif / 2 )->GetAlt( );
        if ( ( iAltL != 0 ) && ( iAltR != 0 ) )
            _iAlt4 = ( iAlt3 + iAlt4 + iAltL + iAltR ) / 4 + ( RandDist( xDif ) + RandDist( yDif ) ) / 2;
        else if ( iAltL != 0 )
            _iAlt4 = ( iAlt3 + iAlt4 + iAltL ) / 3 + RandDist( xDif );
        else if ( iAltR != 0 )
            _iAlt4 = ( iAlt3 + iAlt4 + iAltR ) / 3 + RandDist( xDif );
        else
            _iAlt4 = ( iAlt3 + iAlt4 ) / 2 + RandDist( xDif );
        _iAlt4 = _iAlt4 < 0 ? 0 : ( _iAlt4 > 100 ? 100 : _iAlt4 );
        pHex->SetAlt( _iAlt4 );
    }

    pHex = GetHex( _x, _y );
    if ( ( _iAlt5 = pHex->GetAlt( ) ) == 0 )
    {
        _iAlt5 = ( _iAlt1 + _iAlt2 + _iAlt3 + _iAlt4 ) / 4 + ( RandDist( xDif ) + RandDist( yDif ) ) / 2;
        _iAlt5 = _iAlt5 < 0 ? 0 : ( _iAlt5 > 100 ? 100 : _iAlt5 );
        pHex->SetAlt( _iAlt5 );
    }

    InitSquare( x1, y1, _x, _y, iAlt1, _iAlt1, _iAlt2, _iAlt5 );
    InitSquare( _x, y1, x2, _y, _iAlt1, iAlt2, _iAlt5, _iAlt3 );
    InitSquare( x1, _y, _x, y2, _iAlt2, _iAlt5, iAlt3, _iAlt4 );
    InitSquare( _x, _y, x2, y2, _iAlt5, _iAlt3, _iAlt4, iAlt4 );
}

void CGameMap::InitSquarePass2( int x1, int y1, int x2, int y2, int iAlt1, int iAlt2, int iAlt3, int iAlt4 )
{

    if ( ( x1 + 1 >= x2 ) && ( y1 + 1 >= y2 ) )
        return;

    int xDif = x2 - x1;
    int yDif = y2 - y1;
    int _x   = x1 + xDif / 2;
    int _y   = y1 + yDif / 2;
    int _iAlt1, _iAlt2, _iAlt3, _iAlt4, _iAlt5;

    CHex* pHex = GetHex( _x, y1 );
    _iAlt1     = pHex->GetAlt( );
    int iAltL  = GetHex( _x, y1 - yDif / 2 )->GetAlt( );
    int iAltR  = GetHex( _x, y1 + yDif / 2 )->GetAlt( );
    int iNew   = ( iAlt1 + iAlt2 + iAltL + iAltR ) / 4 + ( RandDist( xDif ) + RandDist( yDif ) ) / 4;
    iNew       = ( _iAlt1 + iNew ) / 2;
    _iAlt1     = iNew < _iAlt1 - 10 ? _iAlt1 - 10 : ( iNew > _iAlt1 + 10 ? _iAlt1 + 10 : iNew );
    _iAlt1     = _iAlt1 < 0 ? 0 : ( _iAlt1 > 100 ? 100 : _iAlt1 );
    pHex->SetAlt( _iAlt1 );

    pHex      = GetHex( x1, _y );
    _iAlt2    = pHex->GetAlt( );
    int iAltT = GetHex( x1 - xDif / 2, _y )->GetAlt( );
    int iAltB = GetHex( x1 + xDif / 2, _y )->GetAlt( );
    iNew      = ( iAlt1 + iAlt3 + iAltT + iAltB ) / 4 + ( RandDist( xDif ) + RandDist( yDif ) ) / 4;
    iNew      = ( _iAlt2 + iNew ) / 2;
    _iAlt2    = iNew < _iAlt2 - 10 ? _iAlt2 - 10 : ( iNew > _iAlt2 + 10 ? _iAlt2 + 10 : iNew );
    _iAlt2    = _iAlt2 < 0 ? 0 : ( _iAlt2 > 100 ? 100 : _iAlt2 );
    pHex->SetAlt( _iAlt2 );

    pHex   = GetHex( x2, _y );
    _iAlt3 = pHex->GetAlt( );
    iAltT  = GetHex( x2 - xDif / 2, _y )->GetAlt( );
    iAltB  = GetHex( x2 + xDif / 2, _y )->GetAlt( );
    iNew   = ( iAlt2 + iAlt4 + iAltT + iAltB ) / 4 + ( RandDist( xDif ) + RandDist( yDif ) ) / 4;
    iNew   = ( _iAlt3 + iNew ) / 2;
    _iAlt3 = iNew < _iAlt3 - 10 ? _iAlt3 - 10 : ( iNew > _iAlt3 + 10 ? _iAlt3 + 10 : iNew );
    _iAlt3 = _iAlt3 < 0 ? 0 : ( _iAlt3 > 100 ? 100 : _iAlt3 );
    pHex->SetAlt( _iAlt3 );

    pHex   = GetHex( _x, y2 );
    _iAlt4 = pHex->GetAlt( );
    iAltL  = GetHex( _x, y2 - yDif / 2 )->GetAlt( );
    iAltR  = GetHex( _x, y2 + yDif / 2 )->GetAlt( );
    iNew   = ( iAlt3 + iAlt4 + iAltL + iAltR ) / 4 + ( RandDist( xDif ) + RandDist( yDif ) ) / 4;
    iNew   = ( _iAlt4 + iNew ) / 2;
    _iAlt4 = iNew < _iAlt4 - 10 ? _iAlt4 - 10 : ( iNew > _iAlt4 + 10 ? _iAlt4 + 10 : iNew );
    _iAlt4 = _iAlt4 < 0 ? 0 : ( _iAlt4 > 100 ? 100 : _iAlt4 );
    pHex->SetAlt( _iAlt4 );

    pHex   = GetHex( _x, _y );
    _iAlt5 = pHex->GetAlt( );
    iNew   = ( _iAlt1 + _iAlt2 + _iAlt3 + _iAlt4 ) / 4 + ( RandDist( xDif ) + RandDist( yDif ) ) / 4;
    iNew   = ( _iAlt5 + iNew ) / 2;
    _iAlt5 = iNew < _iAlt5 - 10 ? _iAlt5 - 10 : ( iNew > _iAlt5 + 10 ? _iAlt5 + 10 : iNew );
    _iAlt5 = _iAlt5 < 0 ? 0 : ( _iAlt5 > 100 ? 100 : _iAlt5 );
    pHex->SetAlt( _iAlt5 );

    InitSquarePass2( x1, y1, _x, _y, iAlt1, _iAlt1, _iAlt2, _iAlt5 );
    InitSquarePass2( _x, y1, x2, _y, _iAlt1, iAlt2, _iAlt5, _iAlt3 );
    InitSquarePass2( x1, _y, _x, y2, _iAlt2, _iAlt5, iAlt3, _iAlt4 );
    InitSquarePass2( _x, _y, x2, y2, _iAlt5, _iAlt3, _iAlt4, iAlt4 );
}

void CGameMap::CheckOcean( )
{
    for ( int x = 0; x < m_eX; ++x )
        for ( int y = 0; y < m_eY; ++y )
        {
            CHex* phex = GetHex( x, y );

            if ( CHex::ocean == phex->GetType( ) )
            {
                if ( GetHex( x, y )->GetAlt( ) > CHex::sea_level )
                    GetHex( x, y )->SetAlt( CHex::sea_level );

                if ( GetHex( x + 1, y )->GetAlt( ) > CHex::sea_level )
                    GetHex( x + 1, y )->SetAlt( CHex::sea_level );

                if ( GetHex( x + 1, y + 1 )->GetAlt( ) > CHex::sea_level )
                    GetHex( x + 1, y + 1 )->SetAlt( CHex::sea_level );

                if ( GetHex( x, y + 1 )->GetAlt( ) > CHex::sea_level )
                    GetHex( x, y + 1 )->SetAlt( CHex::sea_level );
            }
        }
}

void CGameMap::CheckAlt( )
{

    // we have x,y in the dec/inc so we are comparing to already changed hexes
    // Now we watch for alt increase. We allow only 1 level OR 2 levels if
    // its a diamond with oppisate points at 2 levels and the in-between ones
    // at the in-between level.
    for ( int x = m_eX - m_iSideSize + 8; x > -m_iSideSize; x-- )
    {
        theApp.m_pCreateGame->GetDlgStatus( )->SetPer(
            PER_WORLD_CHECK + ( ( ( m_eX - m_iSideSize + 8 ) - x ) * PER_NUM_WORLD_CHECK ) / ( m_eX + 8 ) );

        for ( int y = -m_iSideSize; y < m_eY - m_iSideSize + 8; y++ )
        {
            CHex* pHexOn    = GetHex( CHexCoord( x, y ) );
            CHex* pHexTop   = GetHex( CHexCoord( x, y - 1 ) );
            CHex* pHexRight = GetHex( CHexCoord( x + 1, y ) );
            CHex* pHexUR    = GetHex( CHexCoord( x + 1, y - 1 ) );

            // step 1 - if ! water then >= sea_level
            if ( ( !pHexOn->IsWater( ) ) && ( pHexOn->GetAlt( ) < CHex::sea_level ) )
                pHexOn->SetAlt( CHex::sea_level );

            int aAlt[4];
            aAlt[0]     = pHexOn->GetAdjustStep( );
            aAlt[1]     = pHexTop->GetAdjustStep( );
            aAlt[2]     = pHexUR->GetAdjustStep( );
            aAlt[3]     = pHexRight->GetAdjustStep( );
            int iLowest = 255;
            for ( int iInd = 0; iInd < 4; iInd++ )
                if ( aAlt[iInd] < iLowest )
                    iLowest = aAlt[iInd];
            for ( int iInd = 0; iInd < 4; iInd++ ) aAlt[iInd] -= iLowest;
            BOOL bOk = TRUE;
            for ( int iInd = 0; iInd < 4; iInd++ )
                if ( aAlt[iInd] > 1 )
                {
                    bOk = FALSE;
                    break;
                }
            if ( bOk )
                continue;

            // ok, lets check for 2 level diamond
            int iInd = 0;
            for ( ; iInd < 4; iInd++ )
                if ( aAlt[iInd] == 0 )
                    break;
            // +1, +3 need == 1, +2 == 2
            if ( ( aAlt[( iInd + 1 ) & 3] == 1 ) && ( aAlt[( iInd + 3 ) & 3] == 1 ) && ( aAlt[( iInd + 2 ) & 3] == 2 ) )
                continue;

            // too big a jump.
            // we bring pHexOn closer to the others, but within 1 level of pHexTop
            int iMinAlt = ( pHexTop->GetAlt( ) / CHex::map_step - 1 ) * CHex::map_step;
            int iMaxAlt = ( pHexTop->GetAlt( ) / CHex::map_step + 2 ) * CHex::map_step - 1;
            if ( pHexOn->GetAlt( ) < iMinAlt )
                pHexOn->SetAlt( iMinAlt );
            else if ( pHexOn->GetAlt( ) > iMaxAlt )
                pHexOn->SetAlt( iMaxAlt );
            else
                // ok, its ok with top, check upper right
                if ( aAlt[2] - 1 > aAlt[0] )  // pUR > pOn
            {
                int iAlt = ( pHexUR->GetAlt( ) / CHex::map_step - 1 ) * CHex::map_step;
                pHexOn->SetAlt( __min( iMaxAlt, iAlt ) );
            }
            else if ( aAlt[2] < aAlt[0] - 1 )  // pUR < pOn
            {
                int iAlt = ( pHexUR->GetAlt( ) / CHex::map_step + 2 ) * CHex::map_step - 1;
                pHexOn->SetAlt( __max( iMinAlt, iAlt ) );
            }
            else
                // ok, its ok with top, check right
                if ( aAlt[3] - 1 > aAlt[0] )  // pRight > pOn
            {
                int iAlt = ( pHexRight->GetAlt( ) / CHex::map_step - 1 ) * CHex::map_step;
                pHexOn->SetAlt( __min( iMaxAlt, iAlt ) );
            }
            else if ( aAlt[3] < aAlt[0] - 1 )  // pRight < pOn
            {
                int iAlt = ( pHexRight->GetAlt( ) / CHex::map_step + 2 ) * CHex::map_step - 1;
                pHexOn->SetAlt( __max( iMinAlt, iAlt ) );
            }

            if ( pHexOn->GetAlt( ) > CHex::sea_level || GetHex( CHexCoord( x + 1, y ) )->GetAlt( ) > CHex::sea_level ||
                 GetHex( CHexCoord( x + 1, y + 1 ) )->GetAlt( ) > CHex::sea_level ||
                 GetHex( CHexCoord( x, y + 1 ) )->GetAlt( ) > CHex::sea_level )
                pHexOn->SetType( CHex::mountain );
            else
                pHexOn->SetType( CHex::ocean );
        }
    }
}

// For oceans (& lakes) we use the water tiles so it stays flat.
// For rivers we surround the existing tiles and lower the river altitude
void CGameMap::AddCoastlines( )
{

    int lTotal = m_eX * m_eY;

    // first we drop all river hexes (all 4 corners) by 1, but not below sea_level
    CHexCoord _hex( 0, 0 );
    CHex*     pHex = m_pHex;
    for ( int lOn = 0; lOn < lTotal; lOn++ )
    {
        // oceans (and lakes) we convert the edge water
        if ( pHex->GetType( ) == CHex::river )
        {
            if ( !( pHex->GetUnits( ) & CHex::lr ) )
                if ( pHex->GetAlt( ) > CHex::sea_level )
                    pHex->SetAlt( pHex->GetAlt( ) - 1 );

            // above
            CHex* pHexTest = theMap.GetHex( _hex.X( ), _hex.Y( ) - 1 );
            if ( ( !pHexTest->IsWater( ) ) && ( !( pHex->GetUnits( ) & CHex::lr ) ) )
                if ( pHexTest->GetAlt( ) > CHex::sea_level + 1 )
                {
                    pHexTest->SetAlt( pHexTest->GetAlt( ) - 1 );
                    pHexTest->OrUnits( CHex::lr );
                }

            // upper right
            pHexTest = theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) - 1 );
            if ( ( !pHexTest->IsWater( ) ) && ( !( pHex->GetUnits( ) & CHex::lr ) ) )
                if ( pHexTest->GetAlt( ) > CHex::sea_level + 1 )
                {
                    pHexTest->SetAlt( pHexTest->GetAlt( ) - 1 );
                    pHexTest->OrUnits( CHex::lr );
                }

            // right
            pHexTest = theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) );
            if ( ( !pHexTest->IsWater( ) ) && ( !( pHex->GetUnits( ) & CHex::lr ) ) )
                if ( pHexTest->GetAlt( ) > CHex::sea_level + 1 )
                {
                    pHexTest->SetAlt( pHexTest->GetAlt( ) - 1 );
                    pHexTest->OrUnits( CHex::lr );
                }
        }

        pHex++;
        _hex.X( ) += 1;
        if ( _hex.X( ) >= m_eX )
        {
            _hex.X( ) = 0;
            _hex.Y( ) += 1;
        }
    }

    // turn off lr
    pHex = m_pHex;
    for ( int lOn = 0; lOn < lTotal; lOn++ )
    {
        pHex->NandUnits( CHex::lr );
        pHex++;
    }

    // set coastlines
    _hex = CHexCoord( 0, 0 );
    pHex = m_pHex;
    for ( int lOn = 0; lOn < lTotal; lOn++ )
    {
        // oceans (and lakes) we convert the edge water
        if ( pHex->GetType( ) == CHex::ocean )
            for ( int x = -1; x <= 1; x++ )
                for ( int y = -1; y <= 1; y++ )
                {
                    CHex* pHexTest = theMap.GetHex( _hex.X( ) + x, _hex.Y( ) + y );
                    if ( ( !pHexTest->IsWater( ) ) && ( pHexTest->GetType( ) != CHex::coastline ) )
                    {
                        pHex->SetType( CHex::coastline );
                        goto IsCoast;
                    }
                }

        // rivers we put riverbanks around the outside of water
        if ( pHex->GetType( ) == CHex::river )
            for ( int x = -1; x <= 1; x++ )
                for ( int y = -1; y <= 1; y++ )
                {
                    CHex* pHexTest = theMap.GetHex( _hex.X( ) + x, _hex.Y( ) + y );
                    if ( ( !pHexTest->IsWater( ) ) && ( pHexTest->GetType( ) != CHex::coastline ) )
                        pHexTest->SetType( CHex::coastline );
                }

    IsCoast:
        pHex++;
        _hex.X( ) += 1;
        if ( _hex.X( ) >= m_eX )
        {
            _hex.X( ) = 0;
            _hex.Y( ) += 1;
        }
    }

    // we now have a bunch of possibilities where we don't have appropiate art
    // so we walk through again and create additional coastline as needed
    // WE DO THIS TWICE BECAUSE PLACING COASTLINE MAY MAKE ANOTHER PREVIOUS HEX CHANGED
    for ( int iTest = 0; iTest < 2; iTest++ )
    {
        _hex = CHexCoord( 0, 0 );
        pHex = m_pHex;
        for ( int lOn = 0; lOn < lTotal; lOn++ )
        {
            if ( pHex->GetType( ) == CHex::coastline )
            {
                // for any side we have water on, we need coastline or water on the adjoining sides
                //  (set to coastline if it's land). We just blast coastline instead of testing for it.

                CHex* pHexAbove = theMap.GetHex( _hex.X( ), _hex.Y( ) - 1 );
                CHex* pHexRight = theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) );
                CHex* pHexBelow = theMap.GetHex( _hex.X( ), _hex.Y( ) + 1 );
                CHex* pHexLeft  = theMap.GetHex( _hex.X( ) - 1, _hex.Y( ) );

                // above & below
                if ( ( pHexAbove->IsWater( ) ) || ( pHexBelow->IsWater( ) ) )
                {
                    // check left and right
                    if ( !pHexLeft->IsWater( ) )
                        pHexLeft->SetType( CHex::coastline );
                    if ( !pHexRight->IsWater( ) )
                        pHexRight->SetType( CHex::coastline );
                }

                // left & right
                if ( ( pHexLeft->IsWater( ) ) || ( pHexRight->IsWater( ) ) )
                {
                    // check left and right
                    if ( !pHexAbove->IsWater( ) )
                        pHexAbove->SetType( CHex::coastline );
                    if ( !pHexBelow->IsWater( ) )
                        pHexBelow->SetType( CHex::coastline );
                }
            }

            pHex++;
            _hex.X( ) += 1;
            if ( _hex.X( ) >= m_eX )
            {
                _hex.X( ) = 0;
                _hex.Y( ) += 1;
            }
        }
    }

    // one last time through changing. We can have coastline tiles that don't touch
    // water even on a diaganol. This we turn back to plains
    _hex = CHexCoord( 0, 0 );
    pHex = m_pHex;
    for ( int lOn = 0; lOn < lTotal; lOn++ )
    {
        if ( pHex->GetType( ) == CHex::coastline )
        {
            for ( int x = -1; x <= 1; x++ )
                for ( int y = -1; y <= 1; y++ )
                {
                    CHex* pHexTest = theMap.GetHex( _hex.X( ) + x, _hex.Y( ) + y );
                    if ( pHexTest->IsWater( ) )
                        goto HaveWater;
                }
            // ok, we're clean
            if ( pHex->GetAlt( ) <= CHex::sea_level )
                pHex->SetAlt( CHex::sea_level + 1 );
            pHex->SetType( CHex::plain );
            // BUGBUG			pHex->InitType ();
        }

    HaveWater:
        pHex++;
        _hex.X( ) += 1;
        if ( _hex.X( ) >= m_eX )
        {
            _hex.X( ) = 0;
            _hex.Y( ) += 1;
        }
    }

    // now assign sprites
    _hex = CHexCoord( 0, 0 );
    pHex = m_pHex;
    for ( int lOn = 0; lOn < lTotal; lOn++ )
    {
        if ( pHex->GetType( ) == CHex::coastline )
        {
            // we now get a 4-bit number (0 - 15) for water & coastline neighbors
            int iWater = 0, iCoast = 0, iTyp = OCEAN_COAST_OFF;

            // above
            CHex* pHexTest = theMap.GetHex( _hex.X( ), _hex.Y( ) - 1 );
            if ( pHexTest->IsWater( ) )
            {
                if ( pHexTest->GetType( ) == CHex::river )
                    iTyp = RIVER_COAST_OFF;
                iWater |= 1;
            }
            else if ( pHexTest->GetType( ) == CHex::coastline )
                iCoast |= 1;

            // right
            pHexTest = theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) );
            if ( pHexTest->IsWater( ) )
            {
                if ( pHexTest->GetType( ) == CHex::river )
                    iTyp = RIVER_COAST_OFF;
                iWater |= 2;
            }
            else if ( pHexTest->GetType( ) == CHex::coastline )
                iCoast |= 2;

            // bottom
            pHexTest = theMap.GetHex( _hex.X( ), _hex.Y( ) + 1 );
            if ( pHexTest->IsWater( ) )
            {
                if ( pHexTest->GetType( ) == CHex::river )
                    iTyp = RIVER_COAST_OFF;
                iWater |= 4;
            }
            else if ( pHexTest->GetType( ) == CHex::coastline )
                iCoast |= 4;

            // left
            pHexTest = theMap.GetHex( _hex.X( ) - 1, _hex.Y( ) );
            if ( pHexTest->IsWater( ) )
            {
                if ( pHexTest->GetType( ) == CHex::river )
                    iTyp = RIVER_COAST_OFF;
                iWater |= 8;
            }
            else if ( pHexTest->GetType( ) == CHex::coastline )
                iCoast |= 8;

            // if we have water on any border then water makes the decision
            int iIndex = CHex::island;
            switch ( iWater )
            {
            case 1:  // water above
                iIndex = CHex::land_dn;
                break;
            case 2:  // water right
                iIndex = CHex::land_lf;
                break;
            case 3:  // water above & right
                iIndex = CHex::land_ll;
                break;
            case 4:  // water below
                iIndex = CHex::land_up;
                break;
            case 6:  // water right & below
                iIndex = CHex::land_ul;
                break;
            case 8:  // water left
                iIndex = CHex::land_rt;
                break;
            case 9:  // water above & left
                iIndex = CHex::land_lr;
                break;
            case 12:  // water below & left
                iIndex = CHex::land_ur;
                break;

            case 5:   // water above & below (impossible)
            case 7:   // water above, right, & below (impossible)
            case 10:  // water left & right (impossible)
            case 11:  // water above, right, & left (impossible)
            case 13:  // water above, below, & left (impossible)
            case 14:  // water right, below, & left (impossible)
            case 15:  // island
                iIndex = CHex::island;
                break;

            // if no water touching it's an inside corner
            default:
                // see if we are a river coast
                for ( int x = -1; x <= 1; x++ )
                    for ( int y = -1; y <= 1; y++ )
                    {
                        CHex* pHexTest = theMap.GetHex( _hex.X( ) + x, _hex.Y( ) + y );
                        if ( pHexTest->GetType( ) == CHex::river )
                        {
                            iTyp = RIVER_COAST_OFF;
                            break;
                        }
                    }

                switch ( iCoast )
                {
                case 3:
                    iIndex = CHex::water_ur;
                    break;
                case 6:
                    iIndex = CHex::water_lr;
                    break;
                case 7:  // above, right, & below
                    iIndex = CHex::land_lf;
                    break;
                case 11:  // above, right, & left
                    iIndex = CHex::land_dn;
                    break;
                case 12:
                    iIndex = CHex::water_ll;
                    break;
                case 13:  // above, below, & left
                    iIndex = CHex::land_rt;
                    break;
                case 14:  // right, below, and left
                    iIndex = CHex::land_up;
                    break;
                case 9:
                    iIndex = CHex::water_ul;
                    break;

                // error
                default:
                    iIndex = CHex::island;
                    break;
                }
                break;
            }

            // assign the sprite
            pHex->m_psprite = theTerrain.GetSprite( CHex::coastline, iTyp + iIndex );
            if ( pHex->GetAlt( ) < CHex::sea_level )  // if cause of riverbanks
                pHex->SetAlt( CHex::sea_level );
        }

        pHex++;
        _hex.X( ) += 1;
        if ( _hex.X( ) >= m_eX )
        {
            _hex.X( ) = 0;
            _hex.Y( ) += 1;
        }
    }
}


void CGameMap::EliminateSingles( )
{

    // we do this several times because on pass 1 we may have
    // made something a single/finger for pass 2
    for ( int iTest = 0; iTest < 3; iTest++ )
    {
        CHexCoord _hex( 0, 0 );
        CHex*     pHex   = m_pHex;
        int       lTotal = m_eX * m_eY;
        for ( int lOn = 0; lOn < lTotal; lOn++ )
        {
            CHex* apHex[4];
            // get the guys around us
            apHex[0] = theMap.GetHex( _hex.X( ), _hex.Y( ) - 1 );
            apHex[1] = theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) );
            apHex[2] = theMap.GetHex( _hex.X( ), _hex.Y( ) + 1 );
            apHex[3] = theMap.GetHex( _hex.X( ) - 1, _hex.Y( ) );

            // are we alone?
            int iNumMe = 0;
            for ( int iTest = 0; iTest < 4; iTest++ )
                if ( apHex[iTest]->GetType( ) == pHex->GetType( ) )
                    iNumMe++;
            if ( iNumMe == 0 )
            {
                pHex->SetType( apHex[RandNum( 3 )]->GetType( ) );
                // if we changed to/from ocean fix the sea level
                if ( ( pHex->GetType( ) == CHex::lake ) || ( pHex->GetType( ) == CHex::ocean ) )
                {
                    if ( pHex->GetAlt( ) > CHex::sea_level )
                        pHex->SetAlt( CHex::sea_level );
                }
                else if ( pHex->GetAlt( ) < CHex::sea_level )
                    pHex->SetAlt( CHex::sea_level );
            }
            else

                // are we a finger?
                if ( iNumMe == 1 )
            {
                int iOtherType = -1;
                for ( int iTest = 0; iTest < 4; iTest++ )
                    if ( apHex[iTest]->GetType( ) != pHex->GetType( ) )
                    {
                        if ( iOtherType == -1 )
                            iOtherType = apHex[iTest]->GetType( );
                        else
                            goto NextHex;
                    }
                // ok - make the same as the surrounding ones
                TRAP( );
                pHex->SetType( iOtherType );
                // if we changed to/from ocean fix the sea level
                if ( ( pHex->GetType( ) == CHex::lake ) || ( pHex->GetType( ) == CHex::ocean ) )
                {
                    if ( pHex->GetAlt( ) > CHex::sea_level )
                        pHex->SetAlt( CHex::sea_level );
                }
                else if ( pHex->GetAlt( ) < CHex::sea_level )
                    pHex->SetAlt( CHex::sea_level );
            }

        NextHex:
            pHex++;
            _hex.X( ) += 1;
            if ( _hex.X( ) >= m_eX )
            {
                _hex.X( ) = 0;
                _hex.Y( ) += 1;
            }
        }
    }
}


// we walk through and number all of the bodies of water (using bVisible).
// we first look above (including diags) and if we find a match - that's our
// number. Otherwise we start a new body. Then, all bodies with more than
// m_iSideSize * m_iSideSize / 2 blocks remain oceans.
// we assume a max of 256 bodies and just wrap if there are more. 0 is land
void CGameMap::MakeLakes( )
{

    // set all to 0 to start
    int   iNum   = m_eX * m_eY;
    CHex* pHexOn = _GetHex( 0, 0 );
    while ( iNum-- )
    {
        pHexOn->m_bVisible = 0;
        pHexOn++;
    }

    int aiTotal[256];
    memset( aiTotal, 0, sizeof( aiTotal ) );
    int iIndexNext = 1;
    for ( int y = 0; y < m_eY; y++ )
    {
        CHex* pHexOn = GetHex( 0, y );
        for ( int x = 0; x < m_eX; x++ )
        {
            if ( pHexOn->GetType( ) != CHex::ocean )
                pHexOn->m_bVisible = 0;
            else
            {
                CHex* pHexPrev = GetHex( x - 1, y );
                if ( pHexPrev->m_bVisible != 0 )
                    pHexOn->m_bVisible = pHexPrev->m_bVisible;
                else
                {
                    pHexPrev = GetHex( x - 1, y - 1 );
                    if ( pHexPrev->m_bVisible != 0 )
                        pHexOn->m_bVisible = pHexPrev->m_bVisible;
                    else
                    {
                        pHexPrev = GetHex( x, y - 1 );
                        if ( pHexPrev->m_bVisible != 0 )
                            pHexOn->m_bVisible = pHexPrev->m_bVisible;
                        else
                        {
                            pHexPrev = GetHex( x + 1, y - 1 );
                            if ( pHexPrev->m_bVisible != 0 )
                                pHexOn->m_bVisible = pHexPrev->m_bVisible;
                            else
                            {
                                pHexOn->m_bVisible = (BYTE)iIndexNext;
                                iIndexNext++;
                            }
                        }
                    }
                }
                aiTotal[pHexOn->m_bVisible]++;
            }
            pHexOn = _Xinc( pHexOn );
        }

        // ok, if at the end of a line we go water/water & the number changed, fix it
        CHex* pHexStart = GetHex( 0, y );
        CHex* pHexEnd   = GetHex( m_eX - 1, y );
        if ( ( pHexStart->m_bVisible != 0 ) && ( pHexEnd->m_bVisible != 0 ) &&
             ( pHexStart->m_bVisible != pHexEnd->m_bVisible ) )
        {
            BYTE bNew = pHexStart->m_bVisible;
            BYTE bOld = pHexEnd->m_bVisible;
            for ( int x = 0; x < m_eX; x++ )
            {
                if ( pHexStart->m_bVisible == bOld )
                {
                    pHexStart->m_bVisible = bNew;
                    aiTotal[bNew]++;
                }
                pHexStart = _Xinc( pHexStart );
            }
        }
    }

    // we now handle different numbers for water that matches at y/y+1
    //   think water like a V and (m_eY-1)/0
    for ( int y = 0; y < m_eY; y++ )
    {
        CHex* pHexStart = GetHex( 0, y );
        CHex* pHexEnd   = GetHex( 0, y - 1 );

        for ( int x = 0; x < m_eX; x++ )
        {
            if ( ( pHexStart->m_bVisible != 0 ) && ( pHexEnd->m_bVisible != 0 ) &&
                 ( pHexStart->m_bVisible != pHexEnd->m_bVisible ) )
            {
                BYTE bNew    = pHexStart->m_bVisible;
                BYTE bOld    = pHexEnd->m_bVisible;
                iNum         = m_eX * m_eY;
                CHex* pHexOn = _GetHex( 0, 0 );
                while ( iNum-- )
                {
                    if ( pHexOn->m_bVisible == bOld )
                    {
                        pHexOn->m_bVisible = bNew;
                        aiTotal[bNew]++;
                    }
                    pHexOn++;
                }
            }

            pHexStart = _Xinc( pHexStart );
            pHexEnd   = _Xinc( pHexEnd );
        }
    }

    // ok, everyone with a small count becomes a lake
    // set aiTotal[] to TRUE for lakes, FALSE for other
    int  iLake = m_iSideSize * m_iSideSize / 4;
    int* piOn  = aiTotal;
    for ( iNum = 256; iNum > 0; piOn++, iNum-- )
        if ( *piOn < iLake )
            *piOn = TRUE;
        else
            *piOn = FALSE;
    aiTotal[0] = FALSE;

    // now set the lakes & reset m_bVisible
    iNum   = m_eX * m_eY;
    pHexOn = _GetHex( 0, 0 );
    CHexCoord _hex( 0, 0 );
    while ( iNum-- )
    {
        if ( aiTotal[pHexOn->m_bVisible] )
        {
            ASSERT( ( pHexOn->GetType( ) == CHex::ocean ) && ( pHexOn->GetAlt( ) <= CHex::sea_level ) );
            pHexOn->SetType( CHex::lake );
            // BUGBUG			pHexOn->InitType ();

            // change coastlines
            for ( int x = -1; x <= 1; x++ )
                for ( int y = -1; y <= 1; y++ )
                {
                    CHex* pHexTest = theMap.GetHex( _hex.X( ) + x, _hex.Y( ) + y );
                    if ( pHexTest->GetType( ) == CHex::coastline )
                    {
                        int iOff = pHexTest->m_psprite->GetIndex( );
                        if ( ( iOff < LAKE_COAST_OFF ) || ( iOff >= RIVER_COAST_OFF ) )
                            pHexTest->m_psprite = theTerrain.GetSprite( CHex::coastline, iOff + LAKE_COAST_OFF );
                    }
                }
        }
        pHexOn->m_bVisible = 0;

#ifdef _CHEAT
        if ( _bSeeAll )
            pHexOn->m_bVisible = 1;
#endif

        pHexOn++;
        _hex.X( ) += 1;
        if ( _hex.X( ) >= m_eX )
        {
            _hex.X( ) = 0;
            _hex.Y( ) += 1;
        }
    }
}
