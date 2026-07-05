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
#include "SDL2CreateStatus.h"
#include "stdafx.h"
#include "terrain.inl"

// for MakeRiversFlow (flow-accumulation river generation)
// NOTE: must be included before the DEBUG_NEW macro below redefines `new`
#include <queue>
#include <vector>


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

    // NOTE: do NOT bump g_enTerrainEditGen here. Every path out of SetType calls
    // SetVisibleType (line ~72 for water/city/road, line ~119 for land), and
    // SetVisibleType already routes through g_enEditHex(x,y) — which both bumps the
    // gen AND records the hex for the incremental patch. Bumping here too added a
    // second, UN-recorded gen tick per hex (gendelta > listsz), which defeated the
    // edit-patch and forced a full ~850ms mesh rebuild on every terrain-type change.

    // if not land - just do it
    // coastline is a water-EDGE type: force-store it like city/road, never run the
    // altitude/slope re-derivation below (which would turn a steep coastal hill/
    // mountain BACK into hill/mountain, defeating AddCoastlines pass-2's corner fill
    // and leaving rock against the water with no shore). [shore-fix]
    // river/lake: same trap — worldgen converting LAND to water must stick, or
    // channels get silent mountain/hill gaps on slope>8 hexes (the hex isn't water
    // yet, so the IsWater() escape below doesn't catch it). [river-fix]
    // ocean: SAME trap, and was the one missing — a would-be-ocean hex beside a
    // mountain has a steep slope, so SetType(ocean) fell into the slope re-derivation
    // and got retyped to mountain/hill/rough = flat land, overriding the ocean (big
    // flat dirt areas inside mountain-adjacent water). Latent until c599ca90 turned
    // the slope retype lines from dead no-op comparisons into live assignments. Force-
    // store ocean too so it sticks. [ocean-fix]
    if ( ( iType == city ) || ( iType == road ) || ( iType == coastline ) ||
         ( iType == river ) || ( iType == lake ) || ( iType == ocean ) || IsWater( ) )
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

    // Zero the dims BEFORE freeing the hex array: the AI workers' dims guard
    // (caidata.cpp AiFillHexLiveNoLock) treats eX==0 as the post-Close state,
    // so freeing first left a window where a racing straggler passed the
    // guard and dereferenced the just-freed array (#65 family). Best-effort
    // only (no fence/lock) — the real protection is the straggler handling
    // in myThreadClose — but this ordering makes the guard actually guard.
    m_eX = m_eY = 0;

    delete[] m_pHex;
    m_pHex = NULL;

    m_pLandExit = m_pShipExit = NULL;
}

void CGameMap::GetWorldSize( int iSize, int& iSide, int& iSideSize )
{

    // add 2 blocks for each island & .25 for each ocean-front
    // add 1 block total to liven things up
    float fNumBlks = (float)theGame.m_iWorldGenCount * 1.125f + 1; // -0.4f;  // MP parity: frozen count (Bug 2)

#ifdef _CHEAT
    if ( ( theGame.GetServerNetNum( ) == 0 ) && ( EnGetProfileInt( "Cheat", "ForceOcean", 0 ) ) )
        fNumBlks += 2.0f;
#endif

    // scenarios force an ocean
    if ( theGame.GetScenario( ) >= 0 )
        fNumBlks += 4.0f;
    else
        fNumBlks += 2 + RandNum( 2 ); // randomly add 2-4 blocks? interesting

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
            fNumBlks += 0.5f; // was 0.25. seemed low.
    }

    // determine the size of the world
    int iMin = (int)sqrt( fNumBlks ) + 1;
    iSide    = 1;
    while ( iSide < iMin ) iSide *= 2; // does this have to be *2? could it be +1?

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

// Resolve a world-type preset (EWorldType, chosen on the New Game screen and synced
// via CNetStart) into the parameters the region generator + terrain filler use.
//   fillType   - tile painted by GenerateOcean (-1 ocean, -2 desert, -5 mtn, -6 badlands)
//   oceanStyle - 0 stripe / 1 scatter / 2 grow / 3 island; -1 = random (legacy)
//   bForce     - paint even with <=6 players (an explicit pick always takes effect)
//   bDominant  - cover most of the map (planet themes) vs a random fraction
//   fillerType - bias for leftover blocks in SetRandomTerrainBlock; 0 = default random
struct WorldTypeGen
{
    int  fillType;
    int  oceanStyle;
    bool bForce;
    bool bDominant;
    int  fillerType;
};

static WorldTypeGen GetWorldTypeGen( int wt )
{
    switch ( wt )
    {
    case WORLD_BIG_OCEAN:     return {  -1, 2, true,  false,  0 };
    case WORLD_STRIP_OCEAN:   return {  -1, 0, true,  false,  0 };
    case WORLD_SCATTER_OCEAN: return {  -1, 1, true,  false,  0 };
    case WORLD_ISLANDS:       return {  -1, 3, true,  false,  0 };
    case WORLD_MOUNTAIN:      return {  -5, 2, true,  true,  -5 };
    case WORLD_BADLANDS:      return {  -6, 2, true,  true,  -6 };
    case WORLD_DESERT:        return {  -2, 2, true,  true,  -2 };
    case WORLD_DEFAULT:
    default:                  return {  -1, -1, false, false, 0 };
    }
}

// [wg] world-gen parity trace (cross-platform RAND MISMATCH hunt, board
// 2026-07-02, RE-ARMED 2026-07-04 for the ocean-slider-era divergence): one
// line per build stage — rand-generator fingerprint + map hash (type+alt of
// every hex). Compare a host log against a client log; the first stage whose
// pair differs is where the platforms diverged. Env-gated this time
// (EN_WG_TRACE=1), stderr + ODS like EnMpDiagLog.
bool EnWgTraceOn( )
{
    static int s_iOn = -1;
    if ( s_iOn < 0 )
    {
        const char* sz = getenv( "EN_WG_TRACE" );
        s_iOn = ( sz != NULL && *sz != '\0' && *sz != '0' ) ? 1 : 0;
    }
    return ( s_iOn != 0 );
}

void CGameMap::WgTrace( const char* szStage )
{
    if ( !EnWgTraceOn( ) )
        return;
    DWORD h      = 2166136261UL;
    long  lTotal = (long)m_eX * (long)m_eY;
    for ( long lOn = 0; lOn < lTotal; lOn++ )
    {
        CHex* pHex = m_pHex + lOn;
        h = ( h ^ (DWORD)pHex->GetType( ) ) * 16777619UL;
        h = ( h ^ (DWORD)pHex->GetAlt( ) ) * 16777619UL;
    }
    extern unsigned long long g_myRandCalls;   // rand.cpp EN_RANDTRACE counter — correlates [wg] stages with [randtrace] marks
    char szBuf[144];
    sprintf_s( szBuf, "[wg] %-10s rand=%08lx map=%08lx calls=%llu\n", szStage, (unsigned long)MyRandFP( ),
               (unsigned long)h, (unsigned long long)g_myRandCalls );
    fprintf( stderr, "%s", szBuf );
    OutputDebugStringA( szBuf );
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
    // the max oceans are total blocks - players
    // so theoretically every player could be island
    int  iOceansLeft = iNumBlks - theGame.m_iWorldGenCount;   // MP parity: frozen count (Bug 2)
    int* piBlks      = new int[iNumBlks];
    for ( int iInd = 0; iInd < iNumBlks; iInd++ ) piBlks[iInd] = 0; // set all blocks to 0

    // we need to know the number of island requesting players
    // Island players are poorly named - they get 2 ocean edges
    // and "ocean" players get sometimes (usually?) 1.
    // should be called shore player?
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
    if ( ( theGame.GetServerNetNum( ) == 0 ) && ( EnGetProfileInt( "Cheat", "ForceOcean", 0 ) ) )
    {
        piBlks[0] = -1; // set ocean
        if ( iOceansLeft > 0 )
            iOceansLeft--;
    }
#endif

    // scenarios force an ocean
    if ( theGame.GetScenario( ) >= 0 )
    {
        piBlks[0] = -1; // set ocean
        if ( iOceansLeft > 0 )
            iOceansLeft--;
    }
    DWORD seed = theGame.GetSeed();
    int   seedInt = static_cast<int>( seed );

    // [wg] build inputs — a host/client difference HERE (settings desync, e.g.
    // the Rivers/Ocean sliders) explains a divergence before any stage math does.
    if ( EnWgTraceOn( ) )
    {
        char szWg[176];
        sprintf_s( szWg, "[wg] START seed=%08lx rivers=%ld ocean=%ld wtype=%d plyrs=%d side=%d sz=%d scen=%d\n",
                   (unsigned long)seed, (long)theGame.m_iRivers, (long)theGame.m_iOcean,
                   (int)theGame.m_iWorldType,
                   (int)theGame.m_iWorldGenCount, iSide, iSideSize, iScenario );  // MP parity: frozen count so [wg] plyrs= reflects what world-gen used (Bug 2)
        fprintf( stderr, "%s", szWg );
        OutputDebugStringA( szWg );
    }

    // Generate the dominant terrain regions (ocean by default). The world-type preset
    // chosen on the New Game screen decides WHICH tile gets painted and in WHAT shape;
    // "ocean is just a tile type that can be swapped out". WORLD_DEFAULT keeps the
    // legacy behavior: a random ocean style, and only when there are >6 players.
    WorldTypeGen wtg = GetWorldTypeGen( theGame.m_iWorldType );
    if ( wtg.bForce || theGame.m_iWorldGenCount > 6 )   // MP parity: frozen count (Bug 2)
        GenerateOcean( iNumBlks, piBlks, iSide, wtg.fillType, wtg.oceanStyle, wtg.bDominant, iOceansLeft, theGame );

    // MP world-gen parity bisect mark (newwin greenlit, ocean-gen lane). Between the
    // baseline mark (newworld.cpp, pre-theMap.Init) and the final mark (pre-finalrand),
    // this splits the ocean-block-assignment pass from everything after it (rivers/
    // mountains/coastlines/altitude). With host+client both on EN_RANDTRACE=1 for the
    // same Islands/ocean=74 join: if calls/sum already DIFFER here, the divergence is
    // in GenerateOcean's ocean-count/fill path (the ocean-slider-only code); if they
    // still MATCH here but differ at the final mark, it's downstream of ocean-gen.
    MyRandTrace( "wg: post-GenerateOcean (blocks assigned)" );

    theApp.m_pCreateGame->GetDlgStatus( )->SetPer( PER_WORLD_BLKS );

    // we walk through, if they are an island we put in 2 oceans. If they
    // are ocean front we put in one if they don't have one. We may run
    // out of oceans - though, nothing in life is for certain!!!
    // when we're all done, remaining plots are set to desert/swamp or ocean (or mountains!)
    int iInd       = 0;
    int iPlyrsLeft = theGame.m_iWorldGenCount;   // MP parity: frozen count (Bug 2)
    for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
    {
        // skip to the next blk. Bounds-guarded: iInd can arrive here == iNumBlks
        // from the previous player's increments — don't read piBlks[iNumBlks]
        // (the original checked the bound BEFORE incrementing; a port-era reorder
        // read one past the array). The scarcity break below can leave iInd on an
        // ASSIGNED block — the recovery that follows handles that case too.
        while ( ( iInd < iNumBlks ) && ( piBlks[iInd] != 0 ) )
        {
            if ( iInd >= iNumBlks - iPlyrsLeft )
                break;
            iInd++;
        }

        // out of blocks, or the scarcity break left us on an assigned block.
        // Find a real one instead of overwriting it — overwriting could give two
        // players the same start block (likely with dominant planet themes that
        // pre-fill most of the map, or high player counts).
        if ( ( iInd >= iNumBlks ) || ( piBlks[iInd] != 0 ) )
        {
            int iFree = -1;
            // prefer a truly empty block anywhere on the map
            for ( int i = iNumBlks - 1; i >= 0; i-- )
                if ( piBlks[i] == 0 )
                {
                    iFree = i;
                    break;
                }
            // nothing empty: take any non-player block (ocean/theme terrain)
            if ( iFree < 0 )
                for ( int i = iNumBlks - 1; i >= 0; i-- )
                    if ( piBlks[i] <= 0 )
                    {
                        iFree = i;
                        break;
                    }
            // worst case (more players than blocks) keep the old clamp behavior
            iInd        = ( iFree >= 0 ) ? iFree : ( iNumBlks - 1 );
            iOceansLeft = 0;
        }

        CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
        ASSERT_VALID( pPlr );

        if ( pPlr->m_InitData.GetSupplies( CRaceDef::island ) )
        {
            // spawn "island" player (trys to get 2 oceans)
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

                        // skip any possible assigned tiles (bounds-guarded like the
                        // main skip loop — iInd can be iNumBlks after the ++ above)
                        while ( ( iInd < iNumBlks ) && ( piBlks[iInd] != 0 ) )
                        {
                            if ( iInd >= iNumBlks - iPlyrsLeft )
                            {
                                iInd = iOldInd;
                                break;
                            }
                            iInd++;
                        }
                        if ( iInd >= iNumBlks )  // ran off the end: same fallback
                            iInd = iOldInd;
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
            else
            {
                // why did we run out of oceans?
#ifdef _LOGOUT
                TRAP( );
#endif
            }

            // we assigned [iInd] to the player
            iInd++;
        }

        else
        {
            // Spawn non-island player here (but includes ocean players)
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
                    int blk         = iSide * iSide;
                    int iIndWrapped = iInd;
                    if ( iIndWrapped > blk )
                        iIndWrapped = blk - iIndWrapped;

                    // we put an ocean below if there is a below AND there is no ocean to the left
                    // iInd < iNumBlks guard (ASan-caught): when this player took the LAST
                    // block, iInd is one past the array — the old code READ heap garbage
                    // here (world layout became nondeterministic = cross-platform world
                    // divergence candidate) and, if that garbage was 0, WROTE -1 out of
                    // bounds (heap-metadata corruption; matches a malloc_consolidate abort
                    // seen once in CreateNewWorld). Past the end there IS no "below" block,
                    // so fall through to the wrap-around IndRight branch like the in-bounds
                    // no-below case always did.
                    if ( ( iInd < iNumBlks ) && ( piBlks[iInd] == 0 ) && ( iInd % iSide != 0 ) && ( piBlks[IndLeft( iInd, iSide )] != -1 ) )
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
            if ( iTestOcean > 0 ) // >0 means t hat it is a player
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
                // blocks near players that aren't island players
                SetRandomTerrainBlock( piBlks, iInd );
            }
            if ( piBlks[iInd]  == -1)
                iOceansLeft--;

            iInd++;

            if ( iInd >= iNumBlks )
                break;

            // skip to the next blk
            while ( piBlks[iInd] != 0 )
            {
                if ( iInd >= iNumBlks - iPlyrsLeft )
                    break;
                iInd++;
            }
            if ( piBlks[iInd] != 0 ) // leave if we find an already placed block? not continue?
                break;
        }
    }  // end of setting piBlks[]

    // set remaining blocks (if any) to ocean (-1), desert (-2), swamp (-3), plains (-4), 
    // mountains (-5), badlands (-6)
    while ( iInd < iNumBlks )
    {
        if ( piBlks[iInd] == 0 )
        {
            SetRandomTerrainBlock( piBlks, iInd );
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

    int extraOceanDepth = 0; // TODO: add this to the world config menu when starting a new game

    int _x = 0, _y = 0;
    for ( iInd = 0; iInd < iNumBlks; iInd++ )
    {
        theApp.m_pCreateGame->GetDlgStatus( )->SetPer( PER_WORLD_ASSIGN + ( iInd * PER_NUM_WORLD_ASSIGN ) / iNumBlks );

        switch ( piBlks[iInd] )
        {
        // ocean
        case -1: {
            int blockExtraOceanDepth = RandNum( 60 );
            if ( RandNum( 2 ) == 1 )
                blockExtraOceanDepth *= 0.5f;
            if ( RandNum( 3 ) == 1 )
                blockExtraOceanDepth *= 0.25f;
            if ( RandNum( 4 ) == 1 )
                blockExtraOceanDepth *= 0.0f;

            for ( int iOn = 0; iOn < NUM_OCEAN; iOn++ )
                ( GetHex( CHexCoord( _x * iSideSize + ( ocean[iOn][0] * iSideSize ) / 64,
                                     _y * iSideSize + ( ocean[iOn][1] * iSideSize ) / 64 ) ) )
                    ->Init( __minmax(0,CHex::MaxAlt, ConvertAlt( ocean[iOn][2] / 2 + RandNum(__minmax(0,100, ocean[iOn][2] - (extraOceanDepth + blockExtraOceanDepth)) ), iSideSize ) ));
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

        case -5: {                          // Mountains
            int iAlt = RandNum( 20 ) - 5;  // force different avg altitudes
            for ( int iOn = 0; iOn < NUM_LAND; iOn++ )
            {
                ( GetHex( CHexCoord( _x * iSideSize + ( land[iOn][0] * iSideSize ) / 64,
                                     _y * iSideSize + ( land[iOn][1] * iSideSize ) / 64 ) ) )
                    ->Init( ConvertAlt( iAlt + land[iOn][2] / 2 + RandNum( land[iOn][2] ), iSideSize ) );
            }

            break;
        }
        case -6: { // Badlands
            int iAlt = RandNum( 20 ) - 5;  // force different avg altitudes
            for ( int iOn = 0; iOn < NUM_LAND; iOn++ )
                ( GetHex( CHexCoord( _x * iSideSize + ( land[iOn][0] * iSideSize ) / 64,
                                     _y * iSideSize + ( land[iOn][1] * iSideSize ) / 64 ) ) )
                    ->Init( ConvertAlt( iAlt + land[iOn][2] / 2 + RandNum( land[iOn][2] ), iSideSize ) );
            break;
        }

        // player's starting area
        default: {
            CPlayer* pPlyr = theGame.GetPlayerByPlyr( piBlks[iInd] );
            if ( pPlyr->m_InitData.GetSupplies( CRaceDef::island ) )
                for ( int iOn = 0; iOn < NUM_ISLAND; iOn++ )
                {
                    int iAlt = island[iOn][2]; // island altitude? this was originally going to be an actual island i think
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

#ifdef LOGGINGON
    for ( iInd = 0; iInd < iNumBlks; iInd++ )
    {
        // print out blocktype:
        int blockType = piBlks[iInd];

        char buf[128];
        sprintf_s( buf, "iInd=%d\n", blockType );
        OutputDebugStringA( buf );

        // Let's also sum it up, so we know how many of each we have:
        int oceans = 0;
        int deserts = 0;
        int swamps  = 0;
        int planes    = 0;
        int mountains = 0;
        int badlands = 0;
        int other     = 0;
        switch ( blockType )
        {
            case -1:
            oceans++;
                break;
            case -2:
                deserts++;
                break;
            case -3:
                swamps++;
                break;
            case -4:
                planes++;
                break;
            case -5:
                mountains++;
                break;
            case -6:
                badlands++;
                break;
            default:
                other++;
        }

        // Now print out our numbers with percentages:
        int total = oceans + deserts + swamps + planes + mountains + badlands + other;

        // Avoid divide-by-zero
        float invTotal = ( total > 0 ) ? ( 100.0f / (float)total ) : 0.0f;

        char  buf[512];

        sprintf_s( buf,
                   "Ocean blocks: %d (%.2f%%)\n"
                   "Desert blocks: %d (%.2f%%)\n"
                   "Swamp blocks: %d (%.2f%%)\n"
                   "Plains blocks: %d (%.2f%%)\n"
                   "Mountain blocks: %d (%.2f%%)\n"
                   "Badlands blocks: %d (%.2f%%)\n"
                   "Other blocks: %d (%.2f%%)\n"
                   "Total blocks: %d\n",
                   oceans, oceans * invTotal, deserts, deserts * invTotal, swamps, swamps * invTotal, planes,
                   planes * invTotal, mountains, mountains * invTotal, badlands, badlands * invTotal,
                    other, other * invTotal, total );

    }
#endif

    WgTrace( "blocks" );

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
                    default: {  // land (or mountain/badlands/other?)
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

        // merge in other terrain types, resources, and forests
        int       iOff[4][2] = { 12, 12, 44, 12, 12, 44, 44, 44 };
        const int iTry1[]    = { CHex::rough, CHex::hill, CHex::forest, 0 };
        const int iTry2[]    = { CHex::rough, CHex::rough,  CHex::rough, CHex::hill, CHex::hill,
                              CHex::hill,  CHex::desert, CHex::swamp, CHex::forest, 0 };
        switch ( piBlks[iInd] )
        {
        case -1: {  
            const int iTry[] = { CHex::rough, CHex::plain, CHex::hill, CHex::swamp, CHex::desert };

            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 8 ), iWgDraw2 = RandNum( iSideSize - 8 ), iWgDraw3 = RandNum( 3 );
              MakeTerrain( _x * iSideSize + 4 + iWgDraw1, _y * iSideSize + 4 + iWgDraw2, iTry[iWgDraw3], iSideSize ); }
            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 8 ), iWgDraw2 = RandNum( iSideSize - 8 ), iWgDraw3 = RandNum( 3 );
              MakeTerrain( _x * iSideSize + 4 + iWgDraw1, _y * iSideSize + 4 + iWgDraw2, iTry[iWgDraw3], iSideSize ); }

            // make forest
            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 8 ), iWgDraw2 = RandNum( iSideSize - 8 );
              MakeTerrain( _x * iSideSize + 4 + iWgDraw1, _y * iSideSize + 4 + iWgDraw2, CHex::forest, iSideSize * 2 ); }

            if ( RandNum( 5 ) != 0 )  // make most islands the regular old xil oil islands
            {
                MakeMineral( _x * iSideSize + iSideSize / 2, _y * iSideSize + 4 + iSideSize / 2, CMaterialTypes::copper,
                             iSideSize / 4, 2 );
                MakeMineral( _x * iSideSize + iSideSize / 2, _y * iSideSize + 4 + iSideSize / 2, CMaterialTypes::oil,
                             iSideSize / 4, 2 );
            }
            else if ( RandNum( 4 ) == 0 )  // a iron coal island for variety, a nice rare island
            {
                MakeMineral( _x * iSideSize + iSideSize / 2, _y * iSideSize + 3 + iSideSize / 2, CMaterialTypes::coal,
                             iSideSize / 8, 3 );
                MakeMineral( _x * iSideSize + iSideSize / 2, _y * iSideSize + 3 + iSideSize / 2, CMaterialTypes::iron,
                             iSideSize / 8, 3 );
            }
            else if ( RandNum( 2 ) == 0 )  // individual coal island that CAN have small oil or xil
            {
                MakeMineral( _x * iSideSize + iSideSize / 2, _y * iSideSize + 3 + iSideSize / 2, CMaterialTypes::coal,
                             iSideSize / 8, 2 );

                MakeMineral( _x * iSideSize + iSideSize / 3, _y * iSideSize + 3 + iSideSize / 3, CMaterialTypes::copper,
                             iSideSize / 5, 2 );
                MakeMineral( _x * iSideSize + iSideSize / 3, _y * iSideSize + 3 + iSideSize / 3, CMaterialTypes::oil,
                             iSideSize / 5, 2 );
            }
            else  // individual iron island that CAN have small oil or xil
            {
                MakeMineral( _x * iSideSize + iSideSize / 2, _y * iSideSize + 3 + iSideSize / 2, CMaterialTypes::iron,
                             iSideSize / 8, 2 );

                MakeMineral( _x * iSideSize + iSideSize / 3, _y * iSideSize + 3 + iSideSize / 3, CMaterialTypes::copper,
                             iSideSize / 5, 2 );
                MakeMineral( _x * iSideSize + iSideSize / 3, _y * iSideSize + 3 + iSideSize / 3, CMaterialTypes::oil,
                             iSideSize / 5, 2 );
            }
            break;
        }

        case -2: {  // desert - arid sand/rough, oil & copper hidden under the dunes
            int x = _x * iSideSize + 8 + RandNum( iSideSize - 16 );
            int y = _y * iSideSize + 8 + RandNum( iSideSize - 16 );
            MakeTerrain( x, y, CHex::desert, iSideSize );
            x = _x * iSideSize + 8 + RandNum( iSideSize - 16 );
            y = _y * iSideSize + 8 + RandNum( iSideSize - 16 );
            MakeTerrain( x, y, CHex::desert, iSideSize );
            x = _x * iSideSize + 8 + RandNum( iSideSize - 16 );
            y = _y * iSideSize + 8 + RandNum( iSideSize - 16 );
            MakeTerrain( x, y, CHex::rough, iSideSize );

            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 8 ), iWgDraw2 = RandNum( iSideSize - 8 );
              MakeMineral( _x * iSideSize + 4 + iWgDraw1, _y * iSideSize + 4 + iWgDraw2, CMaterialTypes::oil, iSideSize ); }
            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 8 ), iWgDraw2 = RandNum( iSideSize - 8 );
              MakeMineral( _x * iSideSize + 4 + iWgDraw1, _y * iSideSize + 4 + iWgDraw2, CMaterialTypes::copper, iSideSize / 4 ); }
            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 8 ), iWgDraw2 = RandNum( iSideSize - 8 );
              MakeMineral( _x * iSideSize + 4 + iWgDraw1, _y * iSideSize + 4 + iWgDraw2, CMaterialTypes::oil, iSideSize ); }
            break;
        }

        case -3: {  // swamp
            int x = _x * iSideSize + 8 + RandNum( iSideSize - 16 );
            int y = _y * iSideSize + 8 + RandNum( iSideSize - 16 );
            MakeTerrain( x, y, CHex::plain, iSideSize );
            x = _x * iSideSize + 8 + RandNum( iSideSize - 16 );
            y = _y * iSideSize + 8 + RandNum( iSideSize - 16 );
            MakeTerrain( x, y, CHex::rough, iSideSize );

            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 8 ), iWgDraw2 = RandNum( iSideSize - 8 );
              MakeMineral( _x * iSideSize + 4 + iWgDraw1, _y * iSideSize + 4 + iWgDraw2, CMaterialTypes::oil, iSideSize ); }
            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 8 ), iWgDraw2 = RandNum( iSideSize - 8 );
              MakeMineral( _x * iSideSize + 4 + iWgDraw1, _y * iSideSize + 4 + iWgDraw2, CMaterialTypes::copper, iSideSize / 4 ); }
            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 8 ), iWgDraw2 = RandNum( iSideSize - 8 );
              MakeMineral( _x * iSideSize + 4 + iWgDraw1, _y * iSideSize + 4 + iWgDraw2, CMaterialTypes::oil, iSideSize ); }
            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 8 ), iWgDraw2 = RandNum( iSideSize - 8 );
              MakeMineral( _x * iSideSize + 4 + iWgDraw1, _y * iSideSize + 4 + iWgDraw2, CMaterialTypes::copper, iSideSize / 4 ); }
            break;
        }

        case -4: {  // unoccupied plains
            int xDrop = _x * iSideSize + 8 + RandNum( iSideSize - 16 );
            int yDrop = _y * iSideSize + 8 + RandNum( iSideSize - 16 );
            MakeTerrain( xDrop, yDrop, CHex::forest, iSideSize * 2 );
            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 8 ), iWgDraw2 = RandNum( iSideSize - 8 );
              MakeMineral( _x * iSideSize + 4 + iWgDraw1, _y * iSideSize + 4 + iWgDraw2, CMaterialTypes::coal, iSideSize ); }
            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 8 ), iWgDraw2 = RandNum( iSideSize - 8 );
              MakeMineral( _x * iSideSize + 4 + iWgDraw1, _y * iSideSize + 4 + iWgDraw2, CMaterialTypes::iron, iSideSize / 4 ); }
            xDrop = _x * iSideSize + 8 + RandNum( iSideSize - 16 );
            yDrop = _y * iSideSize + 8 + RandNum( iSideSize - 16 );
            MakeTerrain( xDrop, yDrop, iTry2[RandNum( 8 )], iSideSize );
            break;
        }

        case -5: {  // Mountains - multi-peak mountain ranges / Generate Mountain Block

            // mountain ranges with valleys
            GenerateMountainBlock( _x, iSideSize, _y );

            // Add forest on lower slopes
            int xDrop = _x * iSideSize + 8 + RandNum( iSideSize - 16 );
            int yDrop = _y * iSideSize + 8 + RandNum( iSideSize - 16 );
            MakeTerrain( xDrop, yDrop, CHex::forest, iSideSize );

            // Additional scattered forest patches
            for ( int i = 0; i < 2; i++ )
            {
                xDrop        = _x * iSideSize + 10 + RandNum( iSideSize - 20 );
                yDrop        = _y * iSideSize + 10 + RandNum( iSideSize - 20 );
                CHex* pCheck = GetHex( CHexCoord( xDrop, yDrop ) );
                // Only place forest below treeline
                if ( pCheck->GetAlt( ) < ConvertAlt( 50, iSideSize ) )
                {
                    MakeTerrain( xDrop, yDrop, CHex::forest, iSideSize / 2 );
                }
            }

            // Add minerals scattered throughout
            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 8 ), iWgDraw2 = RandNum( iSideSize - 8 );
              MakeMineral( _x * iSideSize + 4 + iWgDraw1, _y * iSideSize + 1 + iWgDraw2, CMaterialTypes::coal, iSideSize ); }
            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 8 ), iWgDraw2 = RandNum( iSideSize - 8 );
              MakeMineral( _x * iSideSize + 4 + iWgDraw1, _y * iSideSize + 1 + iWgDraw2, CMaterialTypes::iron, iSideSize / 2 ); }

            // Add varied terrain in lower areas
            xDrop = _x * iSideSize + 8 + RandNum( iSideSize - 16 );
            yDrop = _y * iSideSize + 8 + RandNum( iSideSize - 16 );
            MakeTerrain( xDrop, yDrop, iTry2[RandNum( 8 )], iSideSize / 2 );

            break;
        }
        case -6: {

            GenerateBadlandsBlock( _x, iSideSize, _y );

            // Mineral deposits in badlands
            // Coal x2 multiplier deposit
            { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
              const int iWgDraw1 = RandNum( iSideSize - 20 ), iWgDraw2 = RandNum( iSideSize - 20 );
              MakeMineral( _x * iSideSize + 10 + iWgDraw1, _y * iSideSize + 10 + iWgDraw2, CMaterialTypes::coal, iSideSize / 3, 2 ); }

            // Small x5 oil deposit (3-4 hex only)
            if ( RandNum( 3 ) == 0 )
            {
                { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
                  const int iWgDraw1 = RandNum( iSideSize - 40 ), iWgDraw2 = RandNum( iSideSize - 40 ), iWgDraw3 = RandNum( 2 );
                  MakeMineral( _x * iSideSize + 20 + iWgDraw1, _y * iSideSize + 20 + iWgDraw2, CMaterialTypes::oil, 3 + iWgDraw3, 5 ); }
            }

            // Small x2-4 coal deposit
            if ( RandNum( 4 ) == 0 )
            {
                { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
                  const int iWgDraw1 = RandNum( iSideSize - 40 ), iWgDraw2 = RandNum( iSideSize - 40 ), iWgDraw3 = RandNum( 2 ), iWgDraw4 = RandNum( 3 );
                  MakeMineral( _x * iSideSize + 20 + iWgDraw1, _y * iSideSize + 20 + iWgDraw2, CMaterialTypes::coal, 3 + iWgDraw3, 2 + iWgDraw4 ); }
            }

            // Small x2-4 iron deposit
            if ( RandNum( 5 ) == 0 )
            {
                { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
                  const int iWgDraw1 = RandNum( iSideSize - 40 ), iWgDraw2 = RandNum( iSideSize - 40 ), iWgDraw3 = RandNum( 2 ), iWgDraw4 = RandNum( 3 );
                  MakeMineral( _x * iSideSize + 20 + iWgDraw1, _y * iSideSize + 20 + iWgDraw2, CMaterialTypes::iron, 3 + iWgDraw3, 2 + iWgDraw4 ); }
            }

            for ( int x = _x * iSideSize; x < ( _x + 1 ) * iSideSize; x++ )
            {
                for ( int y = _y * iSideSize; y < ( _y + 1 ) * iSideSize; y++ )
                {
                    CHex* pHexUR    = GetHex( CHexCoord( x + 1, y - 1 ) );
                    CHex* pHexTop   = GetHex( CHexCoord( x, y - 1 ) );
                    CHex* pHexRight = GetHex( CHexCoord( x + 1, y ) );

                    CHex* pHexOn = GetHex( CHexCoord( x, y ) );

                    int iSlope  = abs( pHexOn->GetAlt( ) - pHexTop->GetAlt( ) );
                    int iSlope2 = abs( pHexOn->GetAlt( ) - pHexRight->GetAlt( ) );
                    iSlope      = __max( iSlope, iSlope2 );
                    iSlope2     = abs( pHexOn->GetAlt( ) - pHexUR->GetAlt( ) );
                    iSlope      = __max( iSlope, iSlope2 );
                    if ( iSlope > 14 )
                        pHexOn->SetType( CHex::mountain );
                    else if ( iSlope > 7 )
                        pHexOn->SetType( CHex::hill );
                    else if ( iSlope >= 1 )
                        pHexOn->SetType( CHex::rough );
                    else 
                        pHexOn->SetType( CHex::plain );
                }
            }

            break;
        }

        // we randomly put stuff in 3 of the 4 sub-blocks. 1 is always forest
        default: { // player blocks are done here
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

    // cleanup the edges between blocks (done again on ln 1192?)
    // SmoothBlockEdges( iSideSize, iSide );

    WgTrace( "terrain" );

    // at corners of blocks we may place a mountain
    for ( iInd = 0; iInd < iNumBlks; iInd++ )
    {
        CHex* pHexOn = GetHex( CHexCoord( _x, _y ) );
        if ( ( ( pHexOn->GetType( ) != CHex::ocean ) && ( pHexOn->GetAlt( ) < ConvertAlt( 75, iSideSize ) ) ) &&
             ( ( iInd == 0 ) || ( MyRand( ) & 0x0100 ) ) )
        {
            theApp.m_pCreateGame->GetDlgStatus( )->SetPer( PER_WORLD_WATER +
                                                           ( iInd * PER_NUM_WORLD_WATER ) / iNumBlks );

            _x = ( iInd / iSide ) * iSideSize - 4 + RandNum( 8 );
            _y = ( iInd % iSide ) * iSideSize - 4 + RandNum( 8 );

            CHex* pHexOn = GetHex( CHexCoord( _x, _y ) );
            if ( ( pHexOn->GetType( ) != CHex::ocean ) && ( pHexOn->GetAlt( ) < 70 ) && ( MyRand( ) & 0x0100 ) )
            {
                int iAlt = __min( 72, pHexOn->GetAlt( ) + 36 ) + RandNum( 16 );
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
                // now we smooth it out (too smooth i think)
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

    // check ocean before smoothing to get better results
    CheckOcean( );

    // we re-smooth the entire world    
    int iAlt = GetHex( 0, 0 )->GetAlt( );
    InitSquarePass2( 0, 0, m_eX, m_eY, iAlt, iAlt, iAlt, iAlt );
    WgTrace( "pass2" );

    // check for too large an alt increase
    theApp.BaseYield( );
    CheckAlt( );
    WgTrace( "checkalt" );

    // check ocean before smoothing to get better results
    CheckOcean( );
    WgTrace( "checkocean" );

    // put rivers down: random maps use flow-accumulation hydrology (dendritic
    // networks that always reach the sea); scenarios keep the legacy forced
    // seed-and-descend walk so existing scenario layouts stay recognizable.
    if ( !iScenario )
        MakeRiversFlow( piBlks, iSide, iSideSize );
    else
    {
        _x = 8;
        _y = 0;
        for ( iInd = 0; iInd < iNumBlks; iInd++ )
        {
            CHex* pHexOn = GetHex( CHexCoord( _x, _y ) );
            if ( ( pHexOn->GetType( ) != CHex::ocean ) && ( pHexOn->GetType( ) != CHex::river ) &&
                 ( piBlks[IndNext( iInd, iSide )] > 0 ) )
            {
                {
                    BOOL bFound = FALSE;
                    { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
                      const int iWgDraw1 = RandNum( 2 ), iWgDraw2 = RandNum( 4 );
                      MakeRiver( _x + iWgDraw1 - 1, _y + 2 + iWgDraw2, bFound ); }
                }
                {
                    BOOL bFound = FALSE;
                    { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
                      const int iWgDraw1 = RandNum( 4 ), iWgDraw2 = RandNum( 2 );
                      MakeRiver( _x - 2 - iWgDraw1, _y + iWgDraw2 - 1, bFound ); }
                }
                {
                    BOOL bFound = FALSE;
                    { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
                      const int iWgDraw1 = RandNum( 4 ), iWgDraw2 = RandNum( 2 );
                      MakeRiver( _x + 2 + iWgDraw1, _y + iWgDraw2 - 1, bFound ); }
                }
                {
                    BOOL bFound = FALSE;
                    { // sequenced draws: arg-eval order is unspecified C++ (gcc right-to-left vs clang/MSVC differ)
                      const int iWgDraw1 = RandNum( 2 ), iWgDraw2 = RandNum( 4 );
                      MakeRiver( _x + iWgDraw1 - 1, _y - 2 - iWgDraw2, bFound ); }
                }
            }  // if ! water

            _y += iSideSize;
            if ( _y >= m_eY )
            {
                _x += iSideSize;
                _y = 0;
            }
        }
    }

    WgTrace( "rivers" );

    theApp.m_pCreateGame->GetDlgStatus( )->SetMsg( IDS_CHECK_MAP );

    delete[] piBlks;

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

    WgTrace( "xfix" );

    // we assign hill & mountain tiles based on the slope
    for ( int x = 0; x < m_eX; x++ )
        for ( int y = 0; y < m_eY; y++ )
        {
            CHex* pHexOn = GetHex( CHexCoord( x, y ) );
            if ( ( !pHexOn->IsWater( ) ))// && ( pHexOn->GetType( ) != CHex::mountain ) )
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
                else if ( iSlope < 4 && pHexOn->GetType( ) == CHex::mountain )
                {
                    // we dont allow mountain blocks with slow less than 4, 
                    // so lets look at nearby hex's to see what they are
                    if ( pHexRight->GetType( ) != CHex::mountain 
                        && !pHexRight->IsWater( ) )
                    {
                        pHexOn->SetType( pHexRight->GetType( ) );
                    }
                    else if ( pHexTop->GetType( ) != CHex::mountain 
                        && !pHexTop->IsWater( ) )
                    {
                        pHexOn->SetType( pHexTop->GetType( ) );
                    }
                    else if ( pHexUR->GetType( ) != CHex::mountain 
                        && !pHexUR->IsWater( ) )
                    {
                        pHexOn->SetType( pHexUR->GetType( ) );
                    }
                }
            }
        }

    WgTrace( "slopes" );

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

    WgTrace( "tiles" );

    // we now change water tiles on the edge to coastline
    theApp.BaseYield( );
    AddCoastlines( );

    // we now change all small oceans to lakes
    theApp.BaseYield( );
    MakeLakes( );
    WgTrace( "lakes" );

    // FLATTEN LAKES to a single surface level. A connected lake body must be level,
    // but worldgen can leave sea-level (16) holes inside a basin flooded to its spill
    // (e.g. 47): hexes the pool-flood skipped (they were ocean at the time) that
    // MakeLakes then re-typed lake. The result is a lake with ~31-unit altitude holes
    // that render as downward funnel-spikes (ground-truth WGSPIKE dump: t=3 alt=16
    // hexes surrounded by t=3 alt=47). Flood-fill each connected lake component and
    // snap every hex to the component's MODE altitude (the dominant surface), which
    // fills the errant holes (and would lower a stray high hex), making the lake flat.
    {
        const int     NN = m_eX * m_eY;
        std::vector<BYTE> seen( NN, 0 );
        std::vector<int>  comp;
        static const int dx4[4] = { 0, -1, 1, 0 }, dy4[4] = { -1, 0, 0, 1 };
        for ( int i0 = 0; i0 < NN; i0++ )
        {
            if ( seen[i0] || ( m_pHex + i0 )->GetType( ) != CHex::lake )
                continue;
            comp.clear( );
            comp.push_back( i0 );
            seen[i0] = 1;
            int freq[128];
            memset( freq, 0, sizeof( freq ) );
            for ( size_t k = 0; k < comp.size( ); k++ )
            {
                int i = comp[k];
                freq[ ( m_pHex + i )->GetAlt( ) & 127 ]++;
                int x = i & m_iHexMask, y = i >> m_iSideShift;
                for ( int d = 0; d < 4; d++ )
                {
                    int nx = ( x + dx4[d] ) & m_iHexMask, ny = ( y + dy4[d] ) & m_iHexMask;
                    int n  = ( ny << m_iSideShift ) | nx;
                    if ( !seen[n] && ( m_pHex + n )->GetType( ) == CHex::lake )
                    {
                        seen[n] = 1;
                        comp.push_back( n );
                    }
                }
            }
            int modeAlt = 0, modeCnt = -1;
            for ( int a = 0; a < 128; a++ )
                if ( freq[a] > modeCnt ) { modeCnt = freq[a]; modeAlt = a; }
            for ( size_t k = 0; k < comp.size( ); k++ )
                ( m_pHex + comp[k] )->SetAlt( modeAlt );
        }
    }

    // CAP SHORE CLIFFS to <= 2 steps above the adjacent water. High coastal land (incl.
    // coastline tiles) right beside flat water makes the WATER tile's shared corner spike
    // up — "water climbing the hill" + jagged shores (ground truth: lake alt=40 with
    // coastline neighbours at 56-66). The renderer draws <=2-step tiles fine; only >=3-step
    // jumps spike. So clamp every NON-water hex that touches water to (lowest adjacent
    // water level) + 2*map_step. Cliffs are PRESERVED — the big drop just moves one hex
    // inland (water -> 2-step shore hex -> mountain), off the waterline. Deferred apply so
    // each clamp reads original water levels.
    {
        const int NN  = m_eX * m_eY;
        const int CAP = 2 * CHex::map_step;   // 2 steps = 16 alt units
        static const int dx8[8] = { -1, 0, 1, -1, 1, -1, 0, 1 }, dy8[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };
        std::vector<int> newAlt( NN, -1 );
        for ( int i = 0; i < NN; i++ )
        {
            CHex* h = m_pHex + i;
            if ( h->IsWater( ) )
                continue;
            int x = i & m_iHexMask, y = i >> m_iSideShift;
            int loWater = INT_MAX;
            for ( int d = 0; d < 8; d++ )
            {
                int   n  = ( ( ( y + dy8[d] ) & m_iHexMask ) << m_iSideShift ) | ( ( x + dx8[d] ) & m_iHexMask );
                CHex* pN = m_pHex + n;
                if ( pN->GetType( ) == CHex::ocean )
                    loWater = __min( loWater, (int)CHex::sea_level );
                else if ( pN->IsWater( ) )
                    loWater = __min( loWater, pN->GetAlt( ) );
            }
            if ( loWater == INT_MAX )
                continue;   // not a shore hex
            int cap = loWater + CAP;
            if ( h->GetAlt( ) > cap )
                newAlt[i] = cap;
        }
        for ( int i = 0; i < NN; i++ )
            if ( newAlt[i] >= 0 )
                ( m_pHex + i )->SetAlt( newAlt[i] );
    }

    // [WGAUDIT] (EN_WGAUDIT=1) measure the FINAL rendered altitudes to verify the
    // "rivers on slopes / pits & spikes" diagnosis with real numbers, not assumptions.
    if ( getenv( "EN_WGAUDIT" ) )
    {
        const int SL = CHex::sea_level, ST = CHex::map_step;
        int nLand=0, nRiver=0, nLake=0, nOcean=0, nMtn=0, nHill=0;
        int landAltMin=99999, landAltMax=-99999; long long landAltSum=0;
        int nRiverRaised=0;                 // river GetAlt > sea_level+step (renders raised)
        int rivSlope[5]={0,0,0,0,0};        // river max 8-neighbor GetAltDraw step-diff bucket
        int nLakeRaised=0, nLakeFlat=0;     // lake GetAlt > sea_level vs <=
        int nLakeSpikeEdge=0;               // lake hex w/ a land nbr >= 2 steps higher (edge spike)
        int nMtnHillTouchWater=0;           // mountain/hill adjacent to water (the ring artifact)
        int nWaterSteepJunc=0;              // water hex w/ a neighbor GetAltDraw diff > 1 step
        static const int dx8[8]={-1,0,1,-1,1,-1,0,1}, dy8[8]={-1,-1,-1,0,0,1,1,1};
        for ( int y=0; y<m_eY; y++ )
          for ( int x=0; x<m_eX; x++ )
          {
            CHex* h = GetHex( CHexCoord(x,y) );
            int t = h->GetType();
            int ad = h->GetAltDraw();
            BOOL water = h->IsWater();
            if ( t==CHex::mountain ) nMtn++;
            if ( t==CHex::hill ) nHill++;
            int maxStepDiff=0; BOOL touchWater=FALSE, landNbrHi=FALSE;
            for ( int d=0; d<8; d++ ) {
              CHex* n = GetHex( CHexCoord(x+dx8[d], y+dy8[d]) );
              int nd = n->GetAltDraw();
              int sd = abs(ad-nd)/ST;
              if ( sd>maxStepDiff ) maxStepDiff=sd;
              if ( n->IsWater() ) touchWater=TRUE;
              if ( water && !n->IsWater() && (nd-ad)>=2*ST ) landNbrHi=TRUE;
            }
            if ( !water ) {
              nLand++; int a=h->GetAlt(); landAltSum+=a;
              if(a<landAltMin)landAltMin=a; if(a>landAltMax)landAltMax=a;
              if ( (t==CHex::mountain||t==CHex::hill) && touchWater ) nMtnHillTouchWater++;
            }
            if ( t==CHex::river ) {
              nRiver++; if ( h->GetAlt() > SL+ST ) nRiverRaised++;
              rivSlope[ __min(4,maxStepDiff) ]++; if ( maxStepDiff>1 ) nWaterSteepJunc++;
            } else if ( t==CHex::lake ) {
              nLake++; if ( h->GetAlt() > SL ) nLakeRaised++; else nLakeFlat++;
              if ( landNbrHi ) nLakeSpikeEdge++; if ( maxStepDiff>1 ) nWaterSteepJunc++;
            } else if ( t==CHex::ocean ) nOcean++;
          }
        char b[512];
        sprintf_s(b,"[WGAUDIT] land=%d alt[min=%d max=%d avg=%d] | river=%d raised=%d slopeSteps{0:%d 1:%d 2:%d 3:%d 4+:%d} | lake=%d raised=%d flat=%d spikeEdge=%d | ocean=%d | mtn=%d hill=%d touchWater=%d | waterSteepJunc=%d\n",
          nLand, landAltMin, landAltMax, nLand?(int)(landAltSum/nLand):0,
          nRiver, nRiverRaised, rivSlope[0],rivSlope[1],rivSlope[2],rivSlope[3],rivSlope[4],
          nLake, nLakeRaised, nLakeFlat, nLakeSpikeEdge, nOcean, nMtn, nHill, nMtnHillTouchWater, nWaterSteepJunc);
        OutputDebugStringA(b);

        // GROUND TRUTH: dump the worst water-surface spikes with full neighbor detail,
        // so the chevron cause is read off real data, not inferred. type codes:
        // lake=3 hill=4 mtn=5 ocean=6 plain=7 river=8 road=9 rough=10 swamp=11 coast=12.
        // Each line: the spike water hex (type/alt/draw) + its 8 neighbors as type:alt:draw.
        OutputDebugStringA("[WGSPIKE] legend type: lake=3 ocean=6 river=8 coast=12 (others=land); fields type:alt:draw\n");
        int nDumped = 0;
        for ( int y=0; y<m_eY && nDumped<20; y++ )
          for ( int x=0; x<m_eX && nDumped<20; x++ )
          {
            CHex* h = GetHex( CHexCoord(x,y) );
            if ( !h->IsWater() ) continue;
            int ad = h->GetAltDraw();
            int maxd = 0;
            for ( int d=0; d<8; d++ ){ CHex* n=GetHex(CHexCoord(x+dx8[d],y+dy8[d])); int sd=abs(ad-n->GetAltDraw())/ST; if(sd>maxd)maxd=sd; }
            if ( maxd < 3 ) continue;   // only the sharp ones (>= 3 steps = a hard cliff)
            char sb[420]; int off=0;
            off += sprintf_s(sb+off, sizeof(sb)-off, "[WGSPIKE] (%d,%d) t=%d alt=%d draw=%d nbrs ", x,y,h->GetType(),h->GetAlt(),ad);
            for ( int d=0; d<8; d++ ){ CHex* n=GetHex(CHexCoord(x+dx8[d],y+dy8[d])); off += sprintf_s(sb+off,sizeof(sb)-off,"%d:%d:%d ", n->GetType(), n->GetAlt(), n->GetAltDraw()); }
            sb[off++]='\n'; sb[off]=0;
            OutputDebugStringA(sb);
            nDumped++;
          }
    }

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

    WgTrace( "done" );

    // we dup the extra line on the bottom
    int   iNum      = m_eX;
    CHex* pHexStart = _GetHex( 0, 0 );
    CHex* pHexDup   = _GetHex( 0, m_eY - 1 ) + m_eX;
    while ( iNum-- ) *pHexDup++ = *pHexStart++;

    theApp.BaseYield( );

    ASSERT_VALID( this );
}

// Get a random int to specify what to use for 'Generate random block', get random block to generate
void CGameMap::SetRandomTerrainBlock( int* piBlks, int iInd )
{
    // Planet themes (Mountain/Badlands/Desert) bias the leftover land toward the theme
    // terrain so the whole world reads as that type, while still leaving some variety.
    int themeFill = GetWorldTypeGen( theGame.m_iWorldType ).fillerType;
    if ( themeFill != 0 )
    {
        if ( ( MyRand( ) >> 11 ) % 4 != 0 )  // ~75% theme terrain, 25% random variety
        {
            piBlks[iInd] = themeFill;
            return;
        }
    }

    int iRtn = ( MyRand( ) >> 11 ) % 5;  // 0�4
    if ( iRtn == 0 )
    {
        // Rare terrain bucket
        int rare = ( MyRand( ) >> 10 ) & 0x03;  // 0�3

        switch ( rare )
        {
        case 0:
            iRtn = 5;
            break;  // mountains
        case 1:
            iRtn = 6;
            break;  // badlands
        default:
            iRtn = 4;  // fallback to plains
            break;
        }
    }

    piBlks[iInd] = -iRtn;
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

        int maxAlt = CHex::MaxAlt;
        pHexTest->SetAlt( __min( maxAlt, pHexOk->GetAlt( ) - iDiff ) );
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

int CGameMap::MakeMineral( int x, int y, int iTyp, int iSideSize, int multiplier )
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
            theMinerals.InitHex( _hex, iTyp, multiplier );
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

// MakeRiversFlow - flow-accumulation river generation (replaces the old
// seed-at-a-block-corner + greedy-descent MakeRiver walk for random maps;
// scenarios keep the legacy path).
//
// 1. Priority-flood (Barnes) from every ocean hex outward: each land hex gets a
//    "filled" altitude = the water level needed to drain to the ocean. This kills
//    dead-end pits — every hex has a monotonic non-climbing path to the sea.
// 2. While flooding, record each hex's downstream neighbor (the hex that pulled
//    it out of the queue) = D4 flow direction. Flats and basins resolve toward
//    their spill point automatically via pop order.
// 3. Rain 1 unit on every land hex and accumulate downstream (reverse pop order).
// 4. Hexes whose catchment >= threshold T become river — this yields dendritic
//    networks where tributaries merge into trunks, always reaching the ocean.
// 5. Every player block is guaranteed a stream: trace its best drainage line
//    downstream to the network/ocean.
// 6. Where the network crosses a depression, flood the basin to its spill level
//    -> a lake the river flows through (big pools = lake type, puddles = river).
// 7. Bridge diagonal-only water contacts so the land/water X-crossing fix that
//    runs later doesn't sever channels.
//
// Deterministic (no floats, ties broken by hex index) so networked clients
// generate identical maps. All-integer; O(N log N) in the flood.
void CGameMap::MakeRiversFlow( int* piBlks, int iSide, int iSideSize )
{
    const int N = m_eX * m_eY;

    // Rivers slider (New Game screen, 0-100, synced via CNetStart): 60 = the
    // baseline threshold below, 0 = no rivers at all. Applied to iThreshold
    // quadratically further down so the top half of the slider is meaningful.
    int iRivers = __minmax( 0, 100, (int)theGame.m_iRivers );
    if ( iRivers == 0 )
        return;

    theApp.BaseYield( );

    std::vector<int>  filled( N, 0 );
    std::vector<int>  flow( N, -1 );   // downstream hex index, -1 = ocean/none
    std::vector<int>  acc( N, 1 );     // catchment (rain) accumulation
    std::vector<int>  order;           // pop order: ocean-outward, rising filled alt
    std::vector<BYTE> done( N, 0 );
    std::vector<BYTE> mark( N, 0 );    // 1 = river channel, 2 = pool (flooded basin)
    order.reserve( N );

    // min-heap keyed (filled alt | FIFO counter | hex index) — deterministic.
    // The FIFO tie-break matters: equal-altitude cells pop in insertion order, so
    // the flood crosses flats as a true BFS wavefront. With a hex-index tie-break
    // the CheckAlt-clamped ==sea_level coastal flats popped in row-major order,
    // chaining flow ALONG the shore — accumulation then concentrated into ugly
    // shore-parallel rivers sitting cliff-high against the ocean. BFS makes flats
    // drain perpendicular to the shore / toward the basin spill instead.
    // [bit budget: 7 (alt) + 24 (counter) + 24 (index), N <= 2^20]
    std::priority_queue<long long, std::vector<long long>, std::greater<long long> > pq;
    long long llPushed = 0;

    // seed: every ocean hex at the sea surface
    for ( int i = 0; i < N; i++ )
    {
        CHex* pHex = m_pHex + i;
        if ( pHex->GetType( ) == CHex::ocean )
        {
            filled[i] = CHex::sea_level;
            done[i]   = 1;
            pq.push( ( (long long)filled[i] << 48 ) | ( llPushed++ << 24 ) | i );
        }
    }
    if ( pq.empty( ) )  // all-land map: nothing to drain to
        return;

    static const int aDx[4] = { 0, -1, +1, 0 };
    static const int aDy[4] = { -1, 0, 0, +1 };

    // 1+2: priority-flood, recording flow directions
    while ( !pq.empty( ) )
    {
        long long llKey = pq.top( );
        pq.pop( );
        int i = (int)( llKey & 0xFFFFFF );
        order.push_back( i );

        int x = i & m_iHexMask;
        int y = i >> m_iSideShift;
        for ( int d = 0; d < 4; d++ )
        {
            int nx = ( x + aDx[d] ) & m_iHexMask;  // torus wrap (m_eX == m_eY, pow2)
            int ny = ( y + aDy[d] ) & m_iHexMask;
            int n  = ( ny << m_iSideShift ) | nx;
            if ( done[n] )
                continue;
            done[n]   = 1;
            filled[n] = __max( ( m_pHex + n )->GetAlt( ), filled[i] );
            flow[n]   = i;
            pq.push( ( (long long)filled[n] << 48 ) | ( llPushed++ << 24 ) | n );
        }
    }

    // 3: accumulate rain downstream (upstream hexes popped later, so walk reversed)
    for ( int j = (int)order.size( ) - 1; j >= 0; j-- )
    {
        int i = order[j];
        if ( flow[i] >= 0 )
            acc[flow[i]] += acc[i];
    }

    theApp.BaseYield( );

    // 4: threshold — catchment area that spawns a river. Scales with block area so
    // density stays roughly constant across world sizes, then by the Rivers slider:
    // T *= 3600/s² (s=60 → 1x baseline, 100 → ~0.36x ≈ 3x denser, 20 → 9x sparser).
    int iThreshold = __max( 48, ( iSideSize * iSideSize ) / 8 );
    iThreshold     = __max( 8, (int)( (long long)iThreshold * 3600 / ( iRivers * iRivers ) ) );
#ifdef _CHEAT
    iThreshold = EnGetProfileInt( "Cheat", "RiverThreshold", iThreshold );
#endif

    for ( int i = 0; i < N; i++ )
        if ( ( acc[i] >= iThreshold ) && ( ( m_pHex + i )->GetType( ) != CHex::ocean ) )
            mark[i] = 1;

    // 5: guarantee each player block a stream — its best drainage line, traced to
    // the network/ocean (mirrors the old code's rivers-near-players intent)
    int iNumBlks = iSide * iSide;
    for ( int iInd = 0; iInd < iNumBlks; iInd++ )
    {
        if ( piBlks[iInd] <= 0 )
            continue;
        int  bx = ( iInd / iSide ) * iSideSize;
        int  by = ( iInd % iSide ) * iSideSize;
        int  iBest = -1;
        BOOL bHasRiver = FALSE;
        for ( int y = by; y < by + iSideSize; y++ )
            for ( int x = bx; x < bx + iSideSize; x++ )
            {
                int i = ( y << m_iSideShift ) | x;
                if ( mark[i] )
                    bHasRiver = TRUE;
                else if ( ( ( m_pHex + i )->GetType( ) != CHex::ocean ) &&
                          ( ( iBest < 0 ) || ( acc[i] > acc[iBest] ) ) )
                    iBest = i;
            }
        if ( bHasRiver || ( iBest < 0 ) || ( acc[iBest] < 8 ) )
            continue;
        for ( int c = iBest; c >= 0; c = flow[c] )
        {
            if ( mark[c] || ( ( m_pHex + c )->GetType( ) == CHex::ocean ) )
                break;
            mark[c] = 1;
        }
    }

    // widen the big trunks to 2 hexes (lowest-filled unmarked land neighbor)
    for ( int i = 0; i < N; i++ )
    {
        if ( ( mark[i] != 1 ) || ( acc[i] < iThreshold * 6 ) )
            continue;
        int x = i & m_iHexMask;
        int y = i >> m_iSideShift;
        int iSide2 = -1;
        for ( int d = 0; d < 4; d++ )
        {
            int n = ( ( ( y + aDy[d] ) & m_iHexMask ) << m_iSideShift ) | ( ( x + aDx[d] ) & m_iHexMask );
            if ( mark[n] || ( ( m_pHex + n )->GetType( ) == CHex::ocean ) )
                continue;
            if ( ( iSide2 < 0 ) || ( filled[n] < filled[iSide2] ) )
                iSide2 = n;
        }
        if ( iSide2 >= 0 )
            mark[iSide2] = 1;
    }

    // 6: flood depressions the network passes through up to their spill level.
    // every submerged basin hex has filled == spill (> its own alt), so a simple
    // BFS over that equality flood-fills exactly one pool.
    int iPoolCap = ( iSideSize * iSideSize ) / 2;  // same cap MakeLakes uses
    std::vector<int>  aCluster;
    std::vector<BYTE> aPrevMark;  // pre-pool marks, to restore if the basin is too big
    int iNumPools = 0;
    for ( int i = 0; i < N; i++ )
    {
        if ( ( mark[i] != 1 ) || ( filled[i] <= ( m_pHex + i )->GetAlt( ) ) )
            continue;
        int iSpill = filled[i];
        aCluster.clear( );
        aPrevMark.clear( );
        aCluster.push_back( i );
        aPrevMark.push_back( mark[i] );
        mark[i] = 2;
        for ( size_t iOn = 0; iOn < aCluster.size( ) && (int)aCluster.size( ) <= iPoolCap; iOn++ )
        {
            int cx = aCluster[iOn] & m_iHexMask;
            int cy = aCluster[iOn] >> m_iSideShift;
            for ( int d = 0; d < 4; d++ )
            {
                int n = ( ( ( cy + aDy[d] ) & m_iHexMask ) << m_iSideShift ) | ( ( cx + aDx[d] ) & m_iHexMask );
                if ( ( mark[n] == 2 ) || ( filled[n] != iSpill ) ||
                     ( ( m_pHex + n )->GetAlt( ) >= iSpill ) ||
                     ( ( m_pHex + n )->GetType( ) == CHex::ocean ) )
                    continue;
                aCluster.push_back( n );
                aPrevMark.push_back( mark[n] );
                mark[n] = 2;
            }
        }
        if ( (int)aCluster.size( ) > iPoolCap )
        {
            // monster basin: don't flood it, restore and run the channel through
            for ( size_t iOn = 0; iOn < aCluster.size( ); iOn++ )
                mark[aCluster[iOn]] = aPrevMark[iOn];
            continue;
        }
        // flatten the pool to its spill level. Type by size: wide-river pools stay
        // river (a tiny "lake" wart beside a channel reads wrong — only basins
        // bigger than a trunk get lake art). Sea-level pools touching the ocean
        // are just bays — type them ocean so they merge with the sea (and pick up
        // ocean-style shores) instead of becoming lakes glued to the coast.
        int iPoolType = ( aCluster.size( ) <= 12 ) ? CHex::river : CHex::lake;
        if ( iSpill == CHex::sea_level )
            for ( size_t iOn = 0; iOn < aCluster.size( ) && iPoolType != CHex::ocean; iOn++ )
            {
                int cx = aCluster[iOn] & m_iHexMask;
                int cy = aCluster[iOn] >> m_iSideShift;
                for ( int d = 0; d < 4; d++ )
                {
                    int n = ( ( ( cy + aDy[d] ) & m_iHexMask ) << m_iSideShift ) | ( ( cx + aDx[d] ) & m_iHexMask );
                    if ( ( m_pHex + n )->GetType( ) == CHex::ocean )
                    {
                        iPoolType = CHex::ocean;
                        break;
                    }
                }
            }
        for ( size_t iOn = 0; iOn < aCluster.size( ); iOn++ )
        {
            CHex* pHex = m_pHex + aCluster[iOn];
            pHex->SetAlt( iSpill );
            pHex->SetType( iPoolType );
        }
        iNumPools++;
    }

    // 7: bridge diagonal-only water contacts (two channels touching corner-to-
    // corner with land on the anti-diagonal) — the X-crossing fix later would
    // sever one of them. Converting the lower anti-diagonal hex joins them
    // instead. Two sweeps since a fix can create a new contact.
    for ( int iPass = 0; iPass < 2; iPass++ )
        for ( int i = 0; i < N; i++ )
        {
            if ( !mark[i] )
                continue;
            int x = i & m_iHexMask;
            int y = i >> m_iSideShift;
            // forward diagonals only (SE, NE) so each pair is tested once
            for ( int dyDiag = -1; dyDiag <= 1; dyDiag += 2 )
            {
                int iDiag = ( ( ( y + dyDiag ) & m_iHexMask ) << m_iSideShift ) | ( ( x + 1 ) & m_iHexMask );
                if ( !mark[iDiag] && ( ( m_pHex + iDiag )->GetType( ) != CHex::ocean ) )
                    continue;
                int iAnti1 = ( y << m_iSideShift ) | ( ( x + 1 ) & m_iHexMask );
                int iAnti2 = ( ( ( y + dyDiag ) & m_iHexMask ) << m_iSideShift ) | x;
                BOOL bWater1 = mark[iAnti1] || ( ( m_pHex + iAnti1 )->GetType( ) == CHex::ocean );
                BOOL bWater2 = mark[iAnti2] || ( ( m_pHex + iAnti2 )->GetType( ) == CHex::ocean );
                if ( bWater1 || bWater2 )
                    continue;
                mark[( filled[iAnti1] <= filled[iAnti2] ) ? iAnti1 : iAnti2] = 1;
            }
        }

    // write the channels (pools already typed above). SetType(river) force-stores
    // now [river-fix], so steep hexes become waterfalls instead of channel gaps.
    int iNumRiver = 0;
    for ( int i = 0; i < N; i++ )
        if ( mark[i] == 1 )
        {
            ( m_pHex + i )->SetType( CHex::river );
            iNumRiver++;
        }

    // 8: LEVEL the river surface to a smooth descent so a channel no longer carries
    // its LAND altitude. The render makes each hex centre a mesh vertex at
    // GetAltDraw()=max(alt,sea_level); a river typed onto high ground (a hill/mountain
    // the flow crossed) therefore stuck UP as a spike/chevron jutting out of the
    // surrounding water (the "river carved through the mountain but kept its height"
    // bug). The old fix relaxed only ~4 hexes near a mouth (+2/hex), far too weak for
    // a channel crossing high terrain.
    //
    // We now walk EVERY river hex in flood pop order (`order` is ocean-outward / rising
    // fill, so a hex's downstream neighbour is always finalised first) and clamp it to
    // at most +2 alt over its lowest already-set water neighbour, floored at sea_level.
    // One O(N) pass yields a monotonic <=2/hex descent to the sea: spikes are pulled
    // down (the land banks absorb the height), while gentle descents (already <=2/hex)
    // are left untouched. Deterministic — same integer math + pop order on every client.
    int iNumRelaxed = 0;
    for ( size_t j = 0; j < order.size( ); j++ )
    {
        int   i    = order[j];
        CHex* pHex = m_pHex + i;
        if ( pHex->GetType( ) != CHex::river )
            continue;
        int x = i & m_iHexMask;
        int y = i >> m_iSideShift;
        int iLowest = INT_MAX;
        for ( int d = 0; d < 4; d++ )
        {
            int   n  = ( ( ( y + aDy[d] ) & m_iHexMask ) << m_iSideShift ) | ( ( x + aDx[d] ) & m_iHexMask );
            CHex* pN = m_pHex + n;
            if ( pN->GetType( ) == CHex::ocean )
                iLowest = __min( iLowest, (int)CHex::sea_level );
            else if ( pN->IsWater( ) )
                iLowest = __min( iLowest, pN->GetAlt( ) );   // downstream already finalised
        }
        if ( iLowest == INT_MAX )
            continue;
        int iTarget = __max( (int)CHex::sea_level, iLowest + 2 );
        if ( pHex->GetAlt( ) > iTarget )
        {
            pHex->SetAlt( iTarget );
            iNumRelaxed++;
        }
    }

    char szBuf[128];
    sprintf_s( szBuf, "MakeRiversFlow: T=%d river=%d pools=%d relaxed=%d\n", iThreshold, iNumRiver, iNumPools, iNumRelaxed );
    OutputDebugStringA( szBuf );
}

void CGameMap::MakeRiver( int x, int y, BOOL& bFound )
{
    // NOTE: was a single static array holding offsets AND the altitude scratch —
    // the recursive branch calls below clobbered the caller's scratch, corrupting
    // the remaining equal-lowest comparisons after a branch. Offsets stay shared
    // (const), scratch is now per-invocation.
    static const int aOff[4][2] = { 0, -1, -1, 0, +1, 0, 0, +1 };
    int              aAlt[4][3];
    int              iInd, iLowest, iLevel, iFound;
    for ( iInd = 0; iInd < 4; iInd++ )
    {
        aAlt[iInd][0] = aOff[iInd][0];
        aAlt[iInd][1] = aOff[iInd][1];
        aAlt[iInd][2] = 0;
    }

    x = theMap.WrapX( x );
    y = theMap.WrapY( y );

    CHex* pHexOn = _GetHex( x, y );
    if ( pHexOn->IsWater( ) )
        return;
    pHexOn->SetType( CHex::river );

    // if we failed to become water, this isn't a good way to go..
    if ( !pHexOn->IsWater( ) )
        return;

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
                if ( !pHex->IsWater() )
                {
                    if ( !bGotOne )
                    {
                        bGotOne = TRUE;
                        pHex->SetType( CHex::river );
                        iFound = iInd;
                    }
                    else if ( ( !bFound ) && ( MyRand( ) & 0x400 ) )
                    {
                        MakeRiver( x + aAlt[iInd][0], y + aAlt[iInd][1], bFound );
                    }
                }
                else
                {
                    // go until we find an ocean or lake
                    if ( pHex->GetType( ) == CHex::ocean || pHex->GetType( ) == CHex::lake )
                        bFound = true;
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

    // EN_MYRAND_MAX, not stdlib RAND_MAX: MyRand is 15-bit on all platforms now
    // (glibc RAND_MAX+1 also overflowed INT_MAX here, making iVal's sign UB).
    int iVal = ( ( iDist + 1 ) * ( iDist + 1 ) * MyRand( ) ) / ( EN_MYRAND_MAX + 1 );
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
    const int maxHeight = 127;

    if ( ( x1 + 1 >= x2 ) && ( y1 + 1 >= y2 ) )
        return;

    int smoothAmount = 8;

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
    _iAlt1 = iNew < _iAlt1 - smoothAmount ? _iAlt1 - smoothAmount
                                              : ( iNew > _iAlt1 + smoothAmount ? _iAlt1 + smoothAmount : iNew );
    _iAlt1     = _iAlt1 < 0 ? 0 : ( _iAlt1 > maxHeight ? maxHeight : _iAlt1 );
    pHex->SetAlt( _iAlt1 );

    pHex      = GetHex( x1, _y );
    _iAlt2    = pHex->GetAlt( );
    int iAltT = GetHex( x1 - xDif / 2, _y )->GetAlt( );
    int iAltB = GetHex( x1 + xDif / 2, _y )->GetAlt( );
    iNew      = ( iAlt1 + iAlt3 + iAltT + iAltB ) / 4 + ( RandDist( xDif ) + RandDist( yDif ) ) / 4;
    iNew      = ( _iAlt2 + iNew ) / 2;
    _iAlt2    = iNew < _iAlt2 - smoothAmount ? _iAlt2 - smoothAmount : ( iNew > _iAlt2 + smoothAmount ? _iAlt2 + smoothAmount : iNew );
    _iAlt2    = _iAlt2 < 0 ? 0 : ( _iAlt2 > maxHeight ? maxHeight : _iAlt2 );
    pHex->SetAlt( _iAlt2 );

    pHex   = GetHex( x2, _y );
    _iAlt3 = pHex->GetAlt( );
    iAltT  = GetHex( x2 - xDif / 2, _y )->GetAlt( );
    iAltB  = GetHex( x2 + xDif / 2, _y )->GetAlt( );
    iNew   = ( iAlt2 + iAlt4 + iAltT + iAltB ) / 4 + ( RandDist( xDif ) + RandDist( yDif ) ) / 4;
    iNew   = ( _iAlt3 + iNew ) / 2;
    _iAlt3 = iNew < _iAlt3 - smoothAmount ? _iAlt3 - smoothAmount
                                          : ( iNew > _iAlt3 + smoothAmount ? _iAlt3 + smoothAmount : iNew );
    _iAlt3 = _iAlt3 < 0 ? 0 : ( _iAlt3 > maxHeight ? maxHeight : _iAlt3 );
    pHex->SetAlt( _iAlt3 );

    pHex   = GetHex( _x, y2 );
    _iAlt4 = pHex->GetAlt( );
    iAltL  = GetHex( _x, y2 - yDif / 2 )->GetAlt( );
    iAltR  = GetHex( _x, y2 + yDif / 2 )->GetAlt( );
    iNew   = ( iAlt3 + iAlt4 + iAltL + iAltR ) / 4 + ( RandDist( xDif ) + RandDist( yDif ) ) / 4;
    iNew   = ( _iAlt4 + iNew ) / 2;
    _iAlt4 = iNew < _iAlt4 - smoothAmount ? _iAlt4 - smoothAmount : ( iNew > _iAlt4 + 
        smoothAmount ? _iAlt4 + smoothAmount : iNew );
    _iAlt4 = _iAlt4 < 0 ? 0 : ( _iAlt4 > maxHeight ? maxHeight : _iAlt4 );
    pHex->SetAlt( _iAlt4 );

    pHex   = GetHex( _x, _y );
    _iAlt5 = pHex->GetAlt( );
    iNew   = ( _iAlt1 + _iAlt2 + _iAlt3 + _iAlt4 ) / 4 + ( RandDist( xDif ) + RandDist( yDif ) ) / 4;
    iNew   = ( _iAlt5 + iNew ) / 2;
    _iAlt5 = iNew < _iAlt5 - smoothAmount ? _iAlt5 - smoothAmount : ( iNew > _iAlt5 + 
        smoothAmount ? _iAlt5 + smoothAmount : iNew );
    _iAlt5 = _iAlt5 < 0 ? 0 : ( _iAlt5 > maxHeight ? maxHeight : _iAlt5 );
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
                // [ocean-flat] The ocean MUST render flat at sea_level. Earlier this had a
                // ">12 above sea_level -> retype to mountain/hill/rough" branch (added by
                // 5925eeea, the tall-mountain refactor). That converted near-mountain ocean
                // hexes -- which CheckAlt's anti-cliff smoothing had raised toward the
                // adjacent peaks -- into SLOPED LAND fingers sitting in the water (operator:
                // "the mountain edges... notice the slopes"; "oceans and lakes should be
                // flat"). It also DEFEATED the "ocean bleeds into the mountain" effect:
                // because corners are SHARED, clamping the ocean hex's corners to sea_level
                // also pulls the neighbouring mountain's shore corner down to the waterline.
                // So ALWAYS flatten -- never retype. The CAP-SHORE-CLIFFS pass (later in
                // Init) still eases the mountain band behind the waterline to a 2-step shore.
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
                pHexOn->SetAlt( CHex::sea_level); // should this be +1?

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

// Shore-SUPPRESSION test: which neighbours must NOT be converted to coastline.
// IsWater() is river|ocean|lake and EXCLUDES swamp — but swamp is a water-like
// type, so the bank loops treat it as land and paint a shore band right through
// it at river/lake↔swamp borders (the "shore between two waters" bug; rivers >>
// lakes because they meander through swampy lowlands). Treat swamp as water for
// the don't-shore decision only (global IsWater / CanRoad are unchanged).
static inline BOOL IsShoreWater( const CHex* h )
{
    return h->IsWater( ) || ( h->GetType( ) == CHex::swamp );
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
            // NOTE: these guards tested pHex (the river hex, which never gets the
            // lr flag) instead of pHexTest — so a bank hex bordering N river hexes
            // was lowered up to 3 times, notching the banks. Test the bank hex.
            CHex* pHexTest = theMap.GetHex( _hex.X( ), _hex.Y( ) - 1 );
            if ( ( !pHexTest->IsWater( ) ) && ( !( pHexTest->GetUnits( ) & CHex::lr ) ) )
                if ( pHexTest->GetAlt( ) > CHex::sea_level + 1 )
                {
                    pHexTest->SetAlt( pHexTest->GetAlt( ) - 1 );
                    pHexTest->OrUnits( CHex::lr );
                }

            // upper right
            pHexTest = theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) - 1 );
            if ( ( !pHexTest->IsWater( ) ) && ( !( pHexTest->GetUnits( ) & CHex::lr ) ) )
                if ( pHexTest->GetAlt( ) > CHex::sea_level + 1 )
                {
                    pHexTest->SetAlt( pHexTest->GetAlt( ) - 1 );
                    pHexTest->OrUnits( CHex::lr );
                }

            // right
            pHexTest = theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) );
            if ( ( !pHexTest->IsWater( ) ) && ( !( pHexTest->GetUnits( ) & CHex::lr ) ) )
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

    // ORIGIN TRACKING for the facing pass below: in a tight corner (1-wide channel,
    // narrow mouth) EVERY water hex of the passage becomes coastline, so the facing
    // pass sees no IsWater() neighbor at all and falls into the inside-corner
    // (mostly-land) fallback table — painting land art over what is really a water
    // channel, and "island" for the straits it declares impossible. Remember which
    // coastline hexes started as WATER so the facing pass can treat them as wet.
    unsigned char* pbWasWater = new unsigned char[lTotal];
    memset( pbWasWater, 0, lTotal );

    // set coastlines
    _hex = CHexCoord( 0, 0 );
    pHex = m_pHex;
    for ( int lOn = 0; lOn < lTotal; lOn++ )
    {
        // oceans (and lakes) we convert the edge water
        if ( pHex->GetType( ) == CHex::ocean )
        {
            // River/lake MOUTH: an ocean hex that touches another water body must stay
            // open water. At a narrow mouth the banks' land sits diagonally beside every
            // ocean hex in the gap, so the land test below would convert the whole gap to
            // coastline — a shore WALL between the river and the sea ("there shouldn't be
            // shores between river/ocean"). The river's own banks (river rule below)
            // already carry the land-side shore art at the mouth.
            BOOL bMouth = FALSE;
            for ( int x = -1; x <= 1 && !bMouth; x++ )
                for ( int y = -1; y <= 1 && !bMouth; y++ )
                {
                    int iNType = theMap.GetHex( _hex.X( ) + x, _hex.Y( ) + y )->GetType( );
                    if ( iNType == CHex::river || iNType == CHex::lake )
                        bMouth = TRUE;
                }
            if ( bMouth )
            {
                // the gap stays open water, but its LAND banks still need a
                // shore band (the carve-out alone left hard land/water edges
                // around the mouth) — convert them like the river rule below
                for ( int x = -1; x <= 1; x++ )
                    for ( int y = -1; y <= 1; y++ )
                    {
                        CHex* pHexTest = theMap.GetHex( _hex.X( ) + x, _hex.Y( ) + y );
                        if ( ( !IsShoreWater( pHexTest ) ) && ( pHexTest->GetType( ) != CHex::coastline ) )
                            pHexTest->SetType( CHex::coastline );
                    }
            }
            else
            for ( int x = -1; x <= 1; x++ )
                for ( int y = -1; y <= 1; y++ )
                {
                    CHex* pHexTest = theMap.GetHex( _hex.X( ) + x, _hex.Y( ) + y );
                    if ( ( !IsShoreWater( pHexTest ) ) && ( pHexTest->GetType( ) != CHex::coastline ) )
                    {
                        pHex->SetType( CHex::coastline );
                        pbWasWater[lOn] = 1;
                        goto IsCoast;
                    }
                }
        }

        // rivers we put riverbanks around the outside of water
        // (lakes too: MakeRiversFlow creates lake hexes BEFORE this pass, unlike
        // MakeLakes' ocean->lake relabel which inherits ocean's coastline)
        if ( ( pHex->GetType( ) == CHex::river ) || ( pHex->GetType( ) == CHex::lake ) )
            for ( int x = -1; x <= 1; x++ )
                for ( int y = -1; y <= 1; y++ )
                {
                    CHex* pHexTest = theMap.GetHex( _hex.X( ) + x, _hex.Y( ) + y );
                    if ( ( !IsShoreWater( pHexTest ) ) && ( pHexTest->GetType( ) != CHex::coastline ) )
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
                    if ( !IsShoreWater( pHexLeft ) )
                        pHexLeft->SetType( CHex::coastline );
                    if ( !IsShoreWater( pHexRight ) )
                        pHexRight->SetType( CHex::coastline );
                }

                // left & right
                if ( ( pHexLeft->IsWater( ) ) || ( pHexRight->IsWater( ) ) )
                {
                    // check left and right
                    if ( !IsShoreWater( pHexAbove ) )
                        pHexAbove->SetType( CHex::coastline );
                    if ( !IsShoreWater( pHexBelow ) )
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

    // [shore v3] Worldgen pass: DETECT + heal coastline WALLS through water — a
    // `coastline` hex that splits two WATER areas instead of separating water from
    // land (operator repro: lake|coastline|river AND lake|coastline|lake). The ocean
    // loop's bMouth handles ocean mouths; the river/lake bank loop + corner-fill leave
    // these. We look ONLY at the 4 ORTHOGONAL neighbours (counting DIAGONALS turned a
    // normal river bank next to a diagonal lake into water and marched the lake up the
    // river — reverted), and MARK-then-APPLY in one sweep (a converted wall can't
    // cascade into a neighbouring bank). A coastline hex is a wall if:
    //   (a) its orthogonal neighbours hold >=2 DIFFERENT open-water types (lake/river/
    //       ocean junction — catches OPPOSITE and CORNER), OR
    //   (b) LAKE on an OPPOSITE pair (above&below, or left&right) — a same-body lake
    //       neck. Deliberately LAKE-only (same test on OCEAN would eat ocean coves/
    //       straits — a prior regression) and OPPOSITE-pairs-only (spares concave
    //       lake-bay corners, which are perpendicular, not opposite).
    // Heal: convert to water (lake if any lake nbr, else river) at sea level so it
    // draws flat; SetType(river|lake) force-stores. NO re-shore (embedding land-height
    // tiles in water caused altitude-spike chevrons — reverted).
    {
        unsigned char* pbWall = new unsigned char[lTotal];
        memset( pbWall, 0, lTotal );

        _hex = CHexCoord( 0, 0 );
        pHex = m_pHex;
        for ( int lOn = 0; lOn < lTotal; lOn++ )
        {
            if ( pHex->GetType( ) == CHex::coastline )
            {
                CHex* pAbove = theMap.GetHex( _hex.X( ),     _hex.Y( ) - 1 );
                CHex* pBelow = theMap.GetHex( _hex.X( ),     _hex.Y( ) + 1 );
                CHex* pLeft  = theMap.GetHex( _hex.X( ) - 1, _hex.Y( )     );
                CHex* pRight = theMap.GetHex( _hex.X( ) + 1, _hex.Y( )     );
                CHex* aOrtho[4] = { pAbove, pBelow, pLeft, pRight };

                // (a) >=2 distinct open-water types among the 4 orthogonal neighbours
                int  iWaterType = -1;
                BOOL bTwoTypes  = FALSE;
                for ( int n = 0; n < 4; n++ )
                {
                    if ( !aOrtho[n]->IsWater( ) )
                        continue;
                    int iType = aOrtho[n]->GetType( );
                    if ( iWaterType < 0 )
                        iWaterType = iType;
                    else if ( iType != iWaterType )
                        bTwoTypes = TRUE;
                }

                // (b) same-type LAKE neck: lake on an opposite orthogonal pair
                BOOL bLakeNeck =
                    ( ( pAbove->GetType( ) == CHex::lake ) && ( pBelow->GetType( ) == CHex::lake ) ) ||
                    ( ( pLeft->GetType( )  == CHex::lake ) && ( pRight->GetType( ) == CHex::lake ) );

                if ( bTwoTypes || bLakeNeck )
                    pbWall[lOn] = 1;
            }

            pHex++;
            _hex.X( ) += 1;
            if ( _hex.X( ) >= m_eX )
            {
                _hex.X( ) = 0;
                _hex.Y( ) += 1;
            }
        }

        // apply (separate sweep -> decisions used ORIGINAL types, no cascade)
        _hex = CHexCoord( 0, 0 );
        pHex = m_pHex;
        for ( int lOn = 0; lOn < lTotal; lOn++ )
        {
            if ( pbWall[lOn] )
            {
                BOOL bLake = FALSE;
                for ( int dy = -1; dy <= 1; dy++ )
                    for ( int dx = -1; dx <= 1; dx++ )
                        if ( theMap.GetHex( _hex.X( ) + dx, _hex.Y( ) + dy )->GetType( ) == CHex::lake )
                            bLake = TRUE;
                pHex->SetType( bLake ? CHex::lake : CHex::river );
                if ( pHex->GetAlt( ) > CHex::sea_level )
                    pHex->SetAlt( CHex::sea_level );   // draw flat as water
            }

            pHex++;
            _hex.X( ) += 1;
            if ( _hex.X( ) >= m_eX )
            {
                _hex.X( ) = 0;
                _hex.Y( ) += 1;
            }
        }

        delete[] pbWall;
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
    AssignCoastFacings( pbWasWater, FALSE );

    delete[] pbWasWater;
}


// Assign coastline sprite facings from the 4-neighbor configuration.
// pbWasWater[off]=1 marks coastline hexes that were WATER before AddCoastlines
// converted them. At worldgen this is tracked exactly; on load RefitCoastFacings
// rebuilds it from altitude so old saves get correct shores too.
//
// Three-tier decision:
//  1. REAL open-water neighbors decide (original 1996 behavior), plus shapes
//     for the masks 1996 declared "impossible" — they DO occur (bank tongue
//     between river and sea, narrow mouths).
//  2. No real water but this hex was water and has water-origin coastline
//     neighbors: it is part of a tight passage whose ENTIRE channel converted
//     to coastline. Face along the channel; elbow-vs-inner-corner is
//     disambiguated by the inside diagonal.
//  3. Original coastline-neighbor fallback (land-origin inside corners).
//
// bKeepGroup keeps each hex's ocean/lake/river art group from its stored
// facing (load refit — MakeLakes' lake relabel happened post-gen and must not
// be undone); at worldgen the group is derived (lake patched later).
// Returns the number of hexes whose sprite changed.
int CGameMap::AssignCoastFacings( const unsigned char* pbWasWater, BOOL bKeepGroup )
{
    int lTotal   = m_eX * m_eY;
    int nChanged = 0;

    // "wet" = real water OR a coastline hex that was water before conversion.
    // Without the latter, the hexes of a tight passage read as land on every
    // side and pick inside-corner art — grass painted over the channel.
    auto wet = [&]( CHex* p ) -> bool {
        return p->IsWater( ) ||
               ( p->GetType( ) == CHex::coastline && pbWasWater[theMap.GetHexOffPub( p )] );
    };

    CHexCoord _hex( 0, 0 );
    CHex*     pHex = m_pHex;
    for ( int lOn = 0; lOn < lTotal; lOn++ )
    {
        if ( pHex->GetType( ) == CHex::coastline )
        {
            // we now get a 4-bit number (0 - 15) for water & coastline neighbors
            int iWater = 0, iWet = 0, iCoast = 0, iTyp = OCEAN_COAST_OFF;
            BOOL bAnyRiver = FALSE, bAnyOpen = FALSE;   // [shore-arttype] see below

            CHex* apN[4];
            apN[0] = theMap.GetHex( _hex.X( ), _hex.Y( ) - 1 );    // above (bit 1)
            apN[1] = theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) );    // right (bit 2)
            apN[2] = theMap.GetHex( _hex.X( ), _hex.Y( ) + 1 );    // below (bit 4)
            apN[3] = theMap.GetHex( _hex.X( ) - 1, _hex.Y( ) );    // left  (bit 8)
            for ( int iN = 0; iN < 4; iN++ )
            {
                if ( apN[iN]->IsWater( ) )
                {
                    if ( apN[iN]->GetType( ) == CHex::river )
                        bAnyRiver = TRUE;
                    else                                   // ocean or lake = "open" water
                        bAnyOpen = TRUE;
                    iWater |= 1 << iN;
                    iWet   |= 1 << iN;
                }
                else if ( apN[iN]->GetType( ) == CHex::coastline )
                {
                    iCoast |= 1 << iN;
                    if ( pbWasWater[theMap.GetHexOffPub( apN[iN] )] )
                        iWet |= 1 << iN;
                }
            }

            // [shore-arttype] Pick the coast ART GROUP from the OPEN water it borders,
            // not "any river neighbour wins". A coastline that orthogonally touches
            // ocean/lake must wear OPEN-water shore art even when a river also feeds the
            // same junction; only use river-bank art when river is the ONLY water type
            // bordering. Previously a river mouth flowing into the sea painted river-bank
            // art against open ocean (operator: river->ocean junction renders the wrong
            // shore). The facing (iWater) was already type-agnostic; only the texture
            // group discriminated by type. (Lake coasts are relabelled later by MakeLakes
            // and preserved on refit via bKeepGroup, so leaving them as ocean here is safe.)
            if ( bAnyRiver && !bAnyOpen )
                iTyp = RIVER_COAST_OFF;

            // TIER 1: if we have REAL water on any border then water decides
            int iIndex = CHex::island;
            if ( iWater != 0 )
            {
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

            // 1996 declared these masks "impossible" and stamped island art on all
            // of them — but a 1-wide channel produces exactly these (water on
            // opposite sides). No strait art exists, so show the bank transition
            // toward the land side(s); the seam to the far bank feathers at render.
            case 5:   // water above & below — land left & right (strait)
                iIndex = CHex::land_rt;
                break;
            case 7:   // water above, right, & below — land LEFT only
                iIndex = CHex::land_lf;
                break;
            case 10:  // water left & right — land above & below (strait)
                iIndex = CHex::land_up;
                break;
            case 11:  // water above, right, & left — land BELOW only
                iIndex = CHex::land_dn;
                break;
            case 13:  // water above, below, & left — land RIGHT only
                iIndex = CHex::land_rt;
                break;
            case 14:  // water right, below, & left — land ABOVE only
                iIndex = CHex::land_up;
                break;

            case 15:  // wet on all 4 sides
                // A land-origin hex here is a true 1-hex island. A WATER-origin hex
                // only borders land diagonally — island art would drop a land blob
                // into open water; kiss that corner with quarter-land art instead.
                iIndex = CHex::island;
                if ( pbWasWater[lOn] )
                {
                    CHex* pDiag = theMap.GetHex( _hex.X( ) - 1, _hex.Y( ) - 1 );
                    if ( !wet( pDiag ) )
                        iIndex = CHex::land_ul;
                    else if ( !wet( theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) - 1 ) ) )
                        iIndex = CHex::land_ur;
                    else if ( !wet( theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) + 1 ) ) )
                        iIndex = CHex::land_lr;
                    else if ( !wet( theMap.GetHex( _hex.X( ) - 1, _hex.Y( ) + 1 ) ) )
                        iIndex = CHex::land_ll;
                }
                break;

            }
            }
            else if ( pbWasWater[lOn] && iWet != 0 )
            {
                // TIER 2: no real water touching, but this hex WAS water and is
                // part of a tight passage whose entire channel converted to
                // coastline. Face along the channel.

                // the river art group still applies if the passage hugs a river
                for ( int x = -1; x <= 1 && iTyp == OCEAN_COAST_OFF; x++ )
                    for ( int y = -1; y <= 1; y++ )
                        if ( theMap.GetHex( _hex.X( ) + x, _hex.Y( ) + y )->GetType( ) == CHex::river )
                        {
                            iTyp = RIVER_COAST_OFF;
                            break;
                        }

                switch ( iWet )
                {
                case 1:  iIndex = CHex::land_dn; break;   // dead-end stub openings
                case 2:  iIndex = CHex::land_lf; break;
                case 4:  iIndex = CHex::land_up; break;
                case 8:  iIndex = CHex::land_rt; break;

                case 5:  iIndex = CHex::land_rt; break;   // strait (1-wide channel)
                case 10: iIndex = CHex::land_up; break;

                case 7:  iIndex = CHex::land_lf; break;   // T-junction: land 1 side
                case 11: iIndex = CHex::land_dn; break;
                case 13: iIndex = CHex::land_rt; break;
                case 14: iIndex = CHex::land_up; break;

                // adjacent pair = an elbow. The inside diagonal disambiguates:
                // wet = inner corner of a wide water body (keep the original
                // inside-corner art), dry = a 1-wide channel turning around a
                // bank corner (mostly-water art, land kissing the outer corner).
                case 3:   // wet above & right
                    iIndex = wet( theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) - 1 ) )
                                 ? CHex::water_ur : CHex::land_ll;
                    break;
                case 6:   // wet right & below
                    iIndex = wet( theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) + 1 ) )
                                 ? CHex::water_lr : CHex::land_ul;
                    break;
                case 9:   // wet above & left
                    iIndex = wet( theMap.GetHex( _hex.X( ) - 1, _hex.Y( ) - 1 ) )
                                 ? CHex::water_ul : CHex::land_lr;
                    break;
                case 12:  // wet below & left
                    iIndex = wet( theMap.GetHex( _hex.X( ) - 1, _hex.Y( ) + 1 ) )
                                 ? CHex::water_ll : CHex::land_ur;
                    break;

                case 15:  // wet all around: land only on a diagonal — kiss it
                    iIndex = CHex::island;
                    if ( !wet( theMap.GetHex( _hex.X( ) - 1, _hex.Y( ) - 1 ) ) )
                        iIndex = CHex::land_ul;
                    else if ( !wet( theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) - 1 ) ) )
                        iIndex = CHex::land_ur;
                    else if ( !wet( theMap.GetHex( _hex.X( ) + 1, _hex.Y( ) + 1 ) ) )
                        iIndex = CHex::land_lr;
                    else if ( !wet( theMap.GetHex( _hex.X( ) - 1, _hex.Y( ) + 1 ) ) )
                        iIndex = CHex::land_ll;
                    break;
                }
            }
            else
            {
                // TIER 3: if no water touching it's an inside corner
                // (original 1996 fallback, unchanged)

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
            }

            // on refit keep the stored art group (ocean/lake/river): MakeLakes
            // relabeled lake coasts after gen and that must not be undone
            BOOL bAssign = TRUE;
            if ( bKeepGroup )
            {
                int F = ( pHex->m_psprite != NULL &&
                          pHex->m_psprite->GetID( ) == CHex::coastline )
                            ? pHex->m_psprite->GetIndex( ) : -1;
                if ( F >= 0 && F <= 38 )
                    iTyp = ( F / 13 ) * 13;
                else
                    bAssign = FALSE;    // overlay/unknown sprite: leave it alone
            }

            // assign the sprite
            if ( bAssign )
            {
                CTerrainSprite* pNew = theTerrain.GetSprite( CHex::coastline, iTyp + iIndex );
                if ( pNew != NULL && pNew != pHex->m_psprite )
                {
                    pHex->m_psprite = pNew;
                    nChanged++;
                }
                if ( pHex->GetAlt( ) < CHex::sea_level )  // if cause of riverbanks
                    pHex->SetAlt( CHex::sea_level );
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

    return nChanged;
}


// Re-derive coastline facings on LOAD. Saves bake the facing chosen at
// worldgen, so maps generated before the tight-corner fix keep their broken
// shores forever. The worldgen origin info is gone; altitude is the proxy: a
// water-origin coastline hex sits AT sea_level (the facing pass raises sub-sea
// coast to exactly sea_level), land-origin banks were never lowered below
// sea_level+1. Set EN_COASTREFIT=0 to disable.
void CGameMap::RefitCoastFacings( )
{
    const char* e = getenv( "EN_COASTREFIT" );
    if ( e != NULL && *e == '0' )
        return;

    int lTotal = m_eX * m_eY;
    unsigned char* pbWasWater = new unsigned char[lTotal];
    CHex* pHex = m_pHex;
    for ( int lOn = 0; lOn < lTotal; lOn++, pHex++ )
        pbWasWater[lOn] = ( pHex->GetType( ) == CHex::coastline &&
                            pHex->GetAlt( ) <= CHex::sea_level ) ? 1 : 0;

    int nChanged = AssignCoastFacings( pbWasWater, TRUE );
    delete[] pbWasWater;

    char sz[80];
    sprintf( sz, "[COAST-REFIT] re-faced %d coastline hexes on load\n", nChanged );
    OutputDebugString( sz );
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
                // river headwaters are legitimate fingers — eating them shortens
                // every source by a hex per pass (and the TRAP below is fatal)
                if ( pHex->GetType( ) == CHex::river )
                    goto NextHex;
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
