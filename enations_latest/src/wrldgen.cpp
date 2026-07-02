//---------------------------------------------------------------------------
//
//	Copyright (c) 2026 VTier
//	All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "player.h"

#include "stdafx.h"
#include "lastplnt.h"
#include "netapi.h"
#include "terrain.inl"
#include "noise_helpers.inl"

// Max altitude is 127, min is 0

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

// we change the altitude based on how far apart items are
static int ConvertAlt( int iAlt, int iSideSize )
{
    int iDiff = ( iAlt - CHex::sea_level );
    int val = ( CHex::sea_level + ( iDiff / 4 ) + ( iDiff * iSideSize ) / 128 );
    // Ensure we never return an out-of-range altitude (CHex uses 0..127)
    if ( val < 0 )
        val = 0;
    else if ( val > CHex::MaxAlt )
        val = CHex::MaxAlt;

    return val;
}

// Deterministic integer sine/cosine, Q15 (result = sin*32768). World-gen must
// be bit-identical on every platform, and libm sin/cos are NOT — MSVC ucrt,
// glibc and Apple libm differ by ULPs, and one flipped (int) cast on a trig
// result diverges the whole map -> CmdPlay RAND MISMATCH drops every
// cross-platform joiner (3/3 repro on themed worlds, 2026-07-02). Quarter-wave
// table, whole degrees; plenty for terrain shaping (max error <1% of radius).
static const short aSinQ15[91] = {
    0, 572, 1144, 1715, 2286, 2856, 3425, 3993, 4560, 5126,
    5690, 6252, 6813, 7371, 7927, 8481, 9032, 9580, 10126, 10668,
    11207, 11743, 12275, 12803, 13328, 13848, 14365, 14876, 15384, 15886,
    16384, 16877, 17364, 17847, 18324, 18795, 19261, 19720, 20174, 20622,
    21063, 21498, 21926, 22348, 22763, 23170, 23571, 23965, 24351, 24730,
    25102, 25466, 25822, 26170, 26510, 26842, 27166, 27482, 27789, 28088,
    28378, 28660, 28932, 29197, 29452, 29698, 29935, 30163, 30382, 30592,
    30792, 30983, 31164, 31336, 31499, 31651, 31795, 31928, 32052, 32166,
    32270, 32365, 32449, 32524, 32588, 32643, 32688, 32723, 32748, 32763,
    32767 };

static int EnSinDeg( int deg )
{
    deg %= 360;
    if ( deg < 0 )
        deg += 360;
    if ( deg <= 90 )
        return ( aSinQ15[deg] );
    if ( deg <= 180 )
        return ( aSinQ15[180 - deg] );
    if ( deg <= 270 )
        return ( -aSinQ15[deg - 180] );
    return ( -aSinQ15[360 - deg] );
}

static int EnCosDeg( int deg )
{
    return ( EnSinDeg( deg + 90 ) );
}

/// <summary>
/// Returns how close you are to the edge.
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="_x"></param>
/// <param name="_y"></param>
/// <param name="iSideSize"></param>
/// <returns>0 if at edge, 1 if in center plateau</returns>
static float BlockEdgeFactor( int x, int y, int _x, int _y, int iSideSize )
{
    int baseX   = _x * iSideSize;
    int baseY   = _y * iSideSize;
    int maxX    = baseX + iSideSize - 1;
    int maxY    = baseY + iSideSize - 1;
    int distX   = __min( x - baseX, maxX - x );
    int distY   = __min( y - baseY, maxY - y );
    int minDist = __min( distX, distY );

    int   edgeWidth = __max( 6, iSideSize / 6 );
    float t         = (float)minDist / (float)edgeWidth;
    if ( t < 0.0f )
        t = 0.0f;
    if ( t > 1.0f )
        t = 1.0f;

    // Smoothstep for gentle blending near edges
    return t * t * ( 3.0f - 2.0f * t );
}

void CGameMap::GenerateOcean( int iNumBlks, int* piBlks, int iSide, int blockType,
                              int oceanStyle, bool bDominant, int& iOceansLeft, CGame& theGame )
{
    // figure out max ocean blocks
    int   count = theGame.GetAll( ).GetCount( );  // get player count
    float multi = 1.0f;

    if ( count <= 8 && iNumBlks >= 64 )
        multi = 3.0f;
    else if ( count <= 8 && iNumBlks >= 32 )
        multi = 2.0f;
    else if ( count <= 16 && iNumBlks >= 64 )
        multi = 2.0f;
    else if ( count <= 4 && iNumBlks >= 16 )
        multi = 2.0f;

    int maxOceanTileCount = iNumBlks - theGame.GetAll( ).GetCount( ) * multi;

    if ( maxOceanTileCount <= 0 )
        maxOceanTileCount = iNumBlks - theGame.GetAll( ).GetCount( );

    if ( maxOceanTileCount > 0 )
    {

        // How many blocks to paint. Planet themes (bDominant) cover most of the map;
        // the default ocean pass keeps ~20% + player starts free.
        // Ocean-size slider (theGame.m_iOcean 0-100, 50 = baseline): bias the random
        // block count toward the slider while keeping randomness. 50 reproduces the old
        // average (MyRand()%max ~= max/2); 0 ~= no ocean, 100 ~= max. Planet themes
        // (bDominant) keep their intentional near-full coverage.
        int totalCount;
        if ( bDominant )
            totalCount = maxOceanTileCount - ( MyRand( ) % ( 1 + maxOceanTileCount / 4 ) );
        else
        {
            int iOcean = __minmax( 0, 100, (int)theGame.m_iOcean );
            int spread = 1 + maxOceanTileCount;
            totalCount = ( maxOceanTileCount * iOcean / 100 ) + ( MyRand( ) % spread ) - spread / 2;
            totalCount = __minmax( 0, maxOceanTileCount, totalCount );
        }

        // oceanStyle < 0 -> pick at random (legacy WORLD_DEFAULT behavior).
        // 0 = stripe, 1 = scatter, 2 = grow, 3 = grow+island
        if ( oceanStyle < 0 )
            oceanStyle = MyRand( ) % 4;

        if ( oceanStyle == 3 )
        {
            const int TEMP_ISLAND = -999;

            // 1. Pick the ONE island block first
            int islandIndex     = MyRand( ) % iNumBlks;
            piBlks[islandIndex] = TEMP_ISLAND;

            // 2. Initialize the ocean front specifically AT THE COAST of the island
            CList<int, int> oceanFront;

            // Get the 4 neighbors of the island to start the flood
            int coast[4];
            coast[0] = IndPrev( islandIndex, iSide );
            coast[1] = IndNext( islandIndex, iSide );
            coast[2] = IndLeft( islandIndex, iSide );
            coast[3] = IndRight( islandIndex, iSide );

            for ( int i = 0; i < 4; i++ )
            {
                int neighborIndex = coast[i];
                // If the neighbor is empty, make it the "starting line" for the ocean
                if ( piBlks[neighborIndex] == 0 )
                {
                    piBlks[neighborIndex] = blockType;
                    if ( blockType == -1 && iOceansLeft > 0 )
                        iOceansLeft--;
                    oceanFront.AddTail( neighborIndex );
                }
            }

            // 3. Flood outward in a "Ring" pattern
            // Because we use AddTail and remove from Head, it behaves like a Queue (BFS)
            // which naturally creates a circular/diamond expansion.
            int blocksToPlace = totalCount - oceanFront.GetCount( );

            while ( blocksToPlace > 0 && !oceanFront.IsEmpty( ) )
            {
                // IMPORTANT: Pull from the HEAD (Front) of the list to ensure BFS order
                POSITION headPos      = oceanFront.GetHeadPosition( );
                int      currentBlock = oceanFront.GetAt( headPos );
                oceanFront.RemoveAt( headPos );

                int neighbors[4];
                neighbors[0] = IndPrev( currentBlock, iSide );
                neighbors[1] = IndNext( currentBlock, iSide );
                neighbors[2] = IndLeft( currentBlock, iSide );
                neighbors[3] = IndRight( currentBlock, iSide );

                // We still shuffle to keep the "shores" from being perfect squares
                for ( int i = 0; i < 4; ++i )
                {
                    int swapWith        = i + ( MyRand( ) % ( 4 - i ) );
                    int temp            = neighbors[i];
                    neighbors[i]        = neighbors[swapWith];
                    neighbors[swapWith] = temp;
                }

                for ( int i = 0; i < 4 && blocksToPlace > 0; ++i )
                {
                    int nb = neighbors[i];

                    if ( piBlks[nb] == 0 )  // Only fill empty land, skip the TEMP_ISLAND
                    {
                        piBlks[nb] = blockType;
                        if ( blockType == -1 && iOceansLeft > 0 )
                            iOceansLeft--;

                        blocksToPlace--;

                        // Add to the TAIL to ensure we finish the current "ring"
                        // before moving to the next distance layer
                        oceanFront.AddTail( nb );
                    }
                }
            }

            // 4. Reveal the island
            piBlks[islandIndex] = 0;
        }
        else if ( oceanStyle == 2 )
        {
            // 1. Define a temporary value for island blocks.
            // It must be different from 0 (empty) and different from blockType (ocean).
            const int TEMP_ISLAND = -999;

            // Attempt to "grow" the ocean:
            int             seedCount = 1 + MyRand( ) % 3;
            CList<int, int> oceanFront;

            // --- Place initial seed blocks (Unchanged) ---
            for ( int s = 0; s < seedCount; ++s )
            {
                int attempts = 0;
                int seedIndex;
                do {
                    seedIndex = MyRand( ) % iNumBlks;
                    attempts++;
                } while ( piBlks[seedIndex] != 0 && attempts < iNumBlks );

                if ( piBlks[seedIndex] == 0 )
                {
                    piBlks[seedIndex] = blockType;
                    if ( blockType == -1 && iOceansLeft > 0 )
                    {
                        totalCount--;
                        iOceansLeft--;
                    }
                    oceanFront.AddTail( seedIndex );
                }
            }

            // --- Grow ocean ---
            int blocksToPlace = totalCount - seedCount;

            // Increased chance because now it creates permanent barriers
            // A lower number creates large open oceans, higher creates maze-like oceans.
            int islandChance = 5;

            while ( blocksToPlace > 0 && !oceanFront.IsEmpty( ) )
            {
                int frontSize = oceanFront.GetCount( );
                int pickIndex = MyRand( ) % frontSize;

                POSITION pos          = oceanFront.FindIndex( pickIndex );
                int      currentBlock = oceanFront.GetAt( pos );
                oceanFront.RemoveAt( pos );

                int neighbors[4];
                neighbors[0] = IndPrev( currentBlock, iSide );   // above
                neighbors[1] = IndNext( currentBlock, iSide );   // below
                neighbors[2] = IndLeft( currentBlock, iSide );   // left
                neighbors[3] = IndRight( currentBlock, iSide );  // right

                // Shuffle neighbors (Unchanged)
                for ( int i = 0; i < 4; ++i )
                {
                    int swapWith        = i + ( MyRand( ) % ( 4 - i ) );
                    int temp            = neighbors[i];
                    neighbors[i]        = neighbors[swapWith];
                    neighbors[swapWith] = temp;
                }

                // Try to grow into neighbors
                for ( int i = 0; i < 4 && blocksToPlace > 0; ++i )
                {
                    int neighborIndex = neighbors[i];

                    // CRITICAL CHANGE: We only grow if it is 0.
                    // If it is TEMP_ISLAND, the ocean hits a wall and stops.
                    if ( piBlks[neighborIndex] == 0 )
                    {
                        // Check chance to create an island "Wall"
                        if ( MyRand( ) % 100 < islandChance )
                        {
                            // Mark this spot as reserved for an island.
                            // The ocean can NEVER overwrite this now.
                            piBlks[neighborIndex] = TEMP_ISLAND;

                            // OPTIONAL: Make islands "clump" better.
                            // If we made an island block, maybe mark a neighbor as island too
                            // so we don't just get 1-pixel dots.
                            if ( MyRand( ) % 100 < 50 )
                            {
                                int extraLand = IndRight( neighborIndex, iSide );
                                if ( piBlks[extraLand] == 0 )
                                    piBlks[extraLand] = TEMP_ISLAND;
                            }

                            // Do NOT decrement blocksToPlace. We didn't place water.
                            // We want the ocean to find a different path to reach target size.
                            continue;
                        }

                        // Place ocean block
                        piBlks[neighborIndex] = blockType;
                        if ( blockType == -1 && iOceansLeft > 0 )
                            iOceansLeft--;

                        blocksToPlace--;

                        // Add to front (Unchanged logic)
                        if ( MyRand( ) % 100 < 70 )
                        {
                            oceanFront.AddTail( neighborIndex );
                        }
                    }
                }

                // Re-add current (Unchanged)
                if ( blocksToPlace > 0 && MyRand( ) % 100 < 20 )
                {
                    oceanFront.AddTail( currentBlock );
                }

                // Add new seeds (Unchanged)
                if ( blocksToPlace > 0 && MyRand( ) % 100 < 5 )
                {
                    // Spawn a new seed point to accelerate ocean growth
                    int attempts = 0;
                    int newSeedIndex;
                    do {
                        newSeedIndex = MyRand( ) % iNumBlks;
                        attempts++;
                    } while ( ( piBlks[newSeedIndex] != 0 ) && ( attempts < iNumBlks * 2 ) );

                    // Only add if we found an empty block (don't overwrite TEMP_ISLAND or blockType)
                    if ( piBlks[newSeedIndex] == 0 )
                    {
                        piBlks[newSeedIndex] = blockType;
                        if ( blockType == -1 && iOceansLeft > 0 )
                        {
                            totalCount--;
                            iOceansLeft--;
                        }
                        oceanFront.AddTail( newSeedIndex );
                        blocksToPlace--;
                    }
                }
            }

            // --- CLEANUP PHASE ---
            // Iterate over the entire grid and turn the "Reserved Islands" back into normal land (0)
            for ( int i = 0; i < iNumBlks; ++i )
            {
                if ( piBlks[i] == TEMP_ISLAND )
                {
                    piBlks[i] = 0;
                }
            }
        }
        else if ( oceanStyle == 1 )  // Scatter ocean (random placement)
        {
            // Scatter ocean blocks randomly
            for ( int i = 0; i < totalCount; ++i )
            {
                // Find a random unassigned block
                int attempts = 0;
                int blockIndex;
                do {
                    blockIndex = MyRand( ) % iNumBlks;
                    attempts++;
                } while ( piBlks[blockIndex] != 0 && attempts < iNumBlks );

                // If we found an empty block, make it ocean
                if ( piBlks[blockIndex] == 0 )
                {
                    piBlks[blockIndex] = blockType;
                    if ( blockType == -1 && iOceansLeft > 0 )
                        iOceansLeft--;
                }
            }
        }
        else  // simple stripe ocean generation
        {
            for ( int i = 0; i < totalCount; ++i )
            {
                piBlks[i] = blockType;
                if ( blockType == -1 && iOceansLeft > 0 )
                    iOceansLeft--;
            }
        }
    }
}

void CGameMap::GenerateMountainBlock( int _x, int iSideSize, int _y)
{
    int iNumRanges = 1 + RandNum( 2 );

    for ( int iRange = 0; iRange < iNumRanges; iRange++ )
    {
        // Determine if this range is horizontal or vertical
        BOOL bHorizontal = ( MyRand( ) & 0x0100 ) != 0;

        // Create the spine of the mountain range
        int spineStart, spineEnd, perpStart, perpEnd;
        if ( bHorizontal )
        {
            spineStart = _x * iSideSize + 6;
            spineEnd   = ( _x + 1 ) * iSideSize - 6;
            perpStart = perpEnd = _y * iSideSize + RandNum( iSideSize - 12 ) + 6;
        }
        else
        {
            perpStart = perpEnd = _x * iSideSize + RandNum( iSideSize - 12 ) + 6;
            spineStart          = _y * iSideSize + 6;
            spineEnd            = ( _y + 1 ) * iSideSize - 6;
        }

        // More peaks for dramatic ridgelines - Rocky Mountain style
        int iNumPeaks   = 3 + RandNum( 4 );  // 3-6 peaks per range
        int peakSpacing = ( spineEnd - spineStart ) / ( iNumPeaks + 1 );

        // Store peak positions for ridgeline and valley generation
        struct PeakInfo
        {
            int x, y, altitude;
        };
        PeakInfo peaks[10];  // Max peaks we'll generate

        for ( int iPeak = 0; iPeak < iNumPeaks && iPeak < 10; iPeak++ )
        {
            int peakPos  = spineStart + peakSpacing * ( iPeak + 1 ) + RandNum( peakSpacing / 3 ) - peakSpacing / 6;
            int peakPerp = perpStart + RandNum( iSideSize / 6 ) - iSideSize / 12;

            int xPeak, yPeak;
            if ( bHorizontal )
            {
                xPeak = peakPos;
                yPeak = peakPerp;
            }
            else
            {
                xPeak = peakPerp;
                yPeak = peakPos;
            }

            // Rocky Mountain peaks are TALL and sharp
            int baseAlt           = 85 + RandNum( 40 );  // Much higher altitude (85-125 vs old 65-97)
            peaks[iPeak].x        = xPeak;
            peaks[iPeak].y        = yPeak;
            peaks[iPeak].altitude = baseAlt;

            // Create the sharp peak
            CHex* pHexPeak = GetHex( CHexCoord( xPeak, yPeak ) );
            if ( !pHexPeak->IsWater( ) )
            {
                float edgeFactor = BlockEdgeFactor( xPeak, yPeak, _x, _y, iSideSize );
                int   deltaAlt   = ConvertAlt( baseAlt, iSideSize ) - CHex::sea_level;
                int   addAlt     = (int)( (float)deltaAlt * edgeFactor );
                // cant be higher than 127!
                if ( pHexPeak->GetAlt( ) + addAlt > 127 )
                    addAlt = 127 - pHexPeak->GetAlt( );
                
                pHexPeak->SetAlt( __minmax(0, 127, pHexPeak->GetAlt( ) + addAlt) );

                // Create steep, dramatic falloff - sharp peaks
                int iMaxRadius = iSideSize / 10 + RandNum( iSideSize / 16 );

                for ( int radius = 1; radius <= iMaxRadius; radius++ )
                {
                    // Exponential falloff for sharp peaks (steeper than linear)
                    float falloffFactor = 1.0f - ( (float)radius / (float)iMaxRadius );
                    falloffFactor       = falloffFactor * falloffFactor;  // Square for sharper falloff

                    int ringAlt = (int)( baseAlt * falloffFactor ) + 30;  // Keep elevated base

                    // Draw a ring around the peak
                    for ( int angle = 0; angle < 360; angle += 15 )
                    {
                        // Add jaggedness to the ring. Integer Q15 trig — libm
                        // sin/cos here diverged the map across platforms.
                        int   jitter = RandNum( 3 ) - 1;
                        int   rx     = xPeak + ( ( radius + jitter ) * EnCosDeg( angle ) ) / 32768;
                        int   ry     = yPeak + ( ( radius + jitter ) * EnSinDeg( angle ) ) / 32768;

                        CHex* pHex = GetHex( CHexCoord( rx, ry ) );
                        if ( !pHex->IsWater( ) )
                        {
                            int existingAlt = pHex->GetAlt( );
                            int proposedAlt = ConvertAlt( ringAlt + RandNum( 8 ), iSideSize );
                            float edgeFactor = BlockEdgeFactor( rx, ry, _x, _y, iSideSize );

                            // Only raise terrain, don't lower existing peaks
                            if ( proposedAlt > existingAlt )
                            {
                                int delta = proposedAlt - existingAlt;

                                // max size is 127!
                                int newAlt = __minmax( 0, 127, existingAlt + (int)( (float)delta * edgeFactor ) );
                                
                                pHex->SetAlt( newAlt );
                            }
                        }
                    }
                }

                // Add minerals near peaks (unchanged logic)
                int iDif = iMaxRadius / 2;
                if ( RandNum( 4 ) == 1 )
                    MakeMineral( xPeak - iDif, yPeak - iDif, CMaterialTypes::coal, iMaxRadius * 2, 2 );
                if ( RandNum( 4 ) == 1 )
                    MakeMineral( xPeak + iDif, yPeak + iDif, CMaterialTypes::iron, iMaxRadius * 2, 2 );
            }
        }

        // Create sharp ridgelines connecting peaks
        for ( int iRidge = 0; iRidge < iNumPeaks - 1 && iRidge < 9; iRidge++ )
        {
            int x1   = peaks[iRidge].x;
            int y1   = peaks[iRidge].y;
            int x2   = peaks[iRidge + 1].x;
            int y2   = peaks[iRidge + 1].y;
            int alt1 = peaks[iRidge].altitude;
            int alt2 = peaks[iRidge + 1].altitude;

            int ridgeSteps = 20 + RandNum( 15 );

            for ( int step = 0; step <= ridgeSteps; step++ )
            {
                float t = (float)step / (float)ridgeSteps;

                // Interpolate position with slight wander
                int rx = x1 + (int)( ( x2 - x1 ) * t ) + RandNum( 3 ) - 1;
                int ry = y1 + (int)( ( y2 - y1 ) * t ) + RandNum( 3 ) - 1;

                // Ridge altitude - sags slightly in the middle (saddle points)
                float sagFactor = 4.0f * t * ( 1.0f - t );  // Peaks at t=0.5
                int   ridgeAlt  = (int)( alt1 + ( alt2 - alt1 ) * t - sagFactor * 15 );
                ridgeAlt        = __max( ridgeAlt, 55 );  // Keep ridges elevated

                // Create narrow ridge with steep sides
                for ( int w = -2; w <= 2; w++ )
                {
                    int perpX, perpY;
                    if ( bHorizontal )
                    {
                        perpX = rx;
                        perpY = ry + w;
                    }
                    else
                    {
                        perpX = rx + w;
                        perpY = ry;
                    }

                    CHex* pHex = GetHex( CHexCoord( perpX, perpY ) );
                    if ( !pHex->IsWater( ) )
                    {
                        // Sharp dropoff from ridge center
                        int dropoff     = abs( w ) * abs( w ) * 8;  // Quadratic dropoff
                        int hexAlt      = ridgeAlt - dropoff;
                        int proposedAlt = ConvertAlt( hexAlt, iSideSize );

                        if ( proposedAlt > pHex->GetAlt( ) )
                        {
                            float edgeFactor = BlockEdgeFactor( perpX, perpY, _x, _y, iSideSize );
                            int   delta      = proposedAlt - pHex->GetAlt( );
                            // max size is 127!
                            int   newAlt     = __minmax( 0, 127, 
                                pHex->GetAlt( ) + (int)( (float)delta * edgeFactor ) );
                                
                            pHex->SetAlt( newAlt);
                        }
                    }
                }
            }
        }

        // Create U-shaped glacial valleys between peaks
        for ( int iValley = 0; iValley < iNumPeaks - 1 && iValley < 9; iValley++ )
        {
            // Valley runs parallel to ridge but offset to one side
            int valleySide   = ( RandNum( 2 ) == 0 ) ? 1 : -1;
            int valleyOffset = iSideSize / 6 + RandNum( iSideSize / 8 );

            int x1 = peaks[iValley].x;
            int y1 = peaks[iValley].y;
            int x2 = peaks[iValley + 1].x;
            int y2 = peaks[iValley + 1].y;

            // Offset valley start/end perpendicular to ridge
            int vx1, vy1, vx2, vy2;
            if ( bHorizontal )
            {
                vx1 = x1;
                vy1 = y1 + valleySide * valleyOffset;
                vx2 = x2;
                vy2 = y2 + valleySide * valleyOffset;
            }
            else
            {
                vx1 = x1 + valleySide * valleyOffset;
                vy1 = y1;
                vx2 = x2 + valleySide * valleyOffset;
                vy2 = y2;
            }

            // Create cirque (bowl) at valley head
            int cirqueX      = vx1 + RandNum( 6 ) - 3;
            int cirqueY      = vy1 + RandNum( 6 ) - 3;
            int cirqueRadius = 4 + RandNum( 3 );

            for ( int cx = -cirqueRadius; cx <= cirqueRadius; cx++ )
            {
                for ( int cy = -cirqueRadius; cy <= cirqueRadius; cy++ )
                {
                    int dist = (int)sqrt( (float)( cx * cx + cy * cy ) );
                    if ( dist <= cirqueRadius )
                    {
                        CHex* pHex = GetHex( CHexCoord( cirqueX + cx, cirqueY + cy ) );
                        if ( !pHex->IsWater( ) )
                        {
                            // Bowl shape - lower in center, steep walls
                            int bowlAlt = 25 + ( dist * dist * 3 );  // Quadratic rise to edges

                            bowlAlt = bowlAlt ;
                            pHex->SetAlt( __minmax( 0, 127, ConvertAlt( bowlAlt, iSideSize )) );

                            if ( dist <= cirqueRadius / 2 )
                                pHex->SetType( CHex::plain );  // Flat valley floor
                            else
                                pHex->SetType( CHex::rough );
                        }
                    }
                }
            }

            // Create the U-shaped valley path
            int pathSteps   = 35 + RandNum( 20 );
            int valleyWidth = 5 + RandNum( 3 );  // Wider than before for U-shape

            for ( int step = 0; step <= pathSteps; step++ )
            {
                float t = (float)step / (float)pathSteps;

                // Gentle meander (glaciers don't wind as much as rivers).
                // Integer Q15 trig — t*4.0 rad == t*229.18 deg; libm sin here
                // diverged the map across platforms.
                int meanderDeg = (int)( ( (long long)step * 229183 ) / ( (long long)pathSteps * 1000 ) );
                int meander    = ( EnSinDeg( meanderDeg ) * ( iSideSize / 20 ) ) / 32768;

                int baseX = vx1 + (int)( ( vx2 - vx1 ) * t );
                int baseY = vy1 + (int)( ( vy2 - vy1 ) * t );

                if ( bHorizontal )
                    baseY += meander;
                else
                    baseX += meander;

                // U-shaped profile: flat bottom, steep walls
                for ( int w = -valleyWidth - 2; w <= valleyWidth + 2; w++ )
                {
                    int perpX, perpY;
                    if ( bHorizontal )
                    {
                        perpX = baseX;
                        perpY = baseY + w;
                    }
                    else
                    {
                        perpX = baseX + w;
                        perpY = baseY;
                    }

                    CHex* pHex = GetHex( CHexCoord( perpX, perpY ) );
                    if ( !pHex->IsWater( ) )
                    {
                        int absW = abs( w );
                        int valleyAlt;

                        if ( absW <= valleyWidth / 2 )
                        {
                            // Flat valley floor (U-shape characteristic)
                            valleyAlt = 22 + RandNum( 4 );
                            pHex->SetType( CHex::plain );
                        }
                        else if ( absW <= valleyWidth )
                        {
                            // Gentle rise at edges of floor
                            valleyAlt = 25 + ( absW - valleyWidth / 2 ) * 3 + RandNum( 3 );
                            pHex->SetType( CHex::rough );
                        }
                        else
                        {
                            // Steep valley walls (U-shape)
                            int wallDist = absW - valleyWidth;
                            valleyAlt    = 35 + wallDist * wallDist * 4;  // Steep quadratic rise

                            if ( valleyAlt > 55 )
                                pHex->SetType( CHex::hill );
                            else
                                pHex->SetType( CHex::rough );
                        }

                        // Valley floor altitude rises slightly toward the head
                        float headRise = ( 1.0f - t ) * 8.0f;
                        valleyAlt += (int)headRise;

                        int proposedAlt = ConvertAlt( valleyAlt, iSideSize );
                        proposedAlt     = min( 127, proposedAlt );
                        // Valley carves into terrain (can lower it)
                        if ( absW <= valleyWidth || proposedAlt < pHex->GetAlt( ) )
                        {
                            float edgeFactor = BlockEdgeFactor( perpX, perpY, _x, _y, iSideSize );
                            int   curAlt     = pHex->GetAlt( );
                            int   delta      = proposedAlt - curAlt;

                            // max size is 127!
                            int newAlt = __minmax( 0, 127, 
                                curAlt + (int)( (float)delta * edgeFactor ) );
                            pHex->SetAlt( newAlt );
                        }
                    }
                }
            }
        }
    }
}

void CGameMap::GenerateBadlandsBlock( int _x, int iSideSize, int _y)
{
    // Badlands are harsh, eroded desert terrain with sharp relief
    // Features: gullies, gorges, mesas, and broken ridgelines

    int boost = 2 + RandNum( 5 );

    // (a _DEBUG-only "save original alts" snapshot lived here; it was never read
    // and never freed — ~66KB leaked per badlands block per worldgen. Removed.)

    // Calculate how much that boost is worth in "Real Altitude Units"
    // We subtract sea_level at the end to get JUST the change amount
    int  deltaAmount        = ConvertAlt( CHex::sea_level + boost, iSideSize ) - CHex::sea_level;

    bool avoidChangingWater = false;
    for ( int x = _x * iSideSize; x < ( _x + 1 ) * iSideSize; x++ )
    {
        for ( int y = _y * iSideSize; y < ( _y + 1 ) * iSideSize; y++ )
        {
            CHex* pHex = GetHex( CHexCoord( x, y ) );
            if ( !avoidChangingWater  || !pHex->IsWater( ) )
            {
                float edgeFactor = BlockEdgeFactor( x, y, _x, _y, iSideSize );
                int   variation  = RandNum( 5 ) - 2;
                int   delta      = (int)( (float)( deltaAmount + variation ) * edgeFactor );

                int previousAlt = pHex->GetAlt( );
                int newAlt      = previousAlt + delta;
                newAlt          = __minmax(0, 127, newAlt );
                pHex->SetAlt( newAlt );

                int appliedAlt = pHex->GetAlt( );
                if ( appliedAlt < previousAlt )
                {
                    TRACE( "SINKING!  old=%d, delta=%d, new=%d, edgeFactor=%f\n", previousAlt, delta, appliedAlt,
                           edgeFactor );
                }
            }
        }
    }

    // Add broad undulating relief to avoid flatness
    // (dont need this because the ground isn't flat to start with)
    /*
    for ( int x = _x * iSideSize; x < ( _x + 1 ) * iSideSize; x++ )
    {
        for ( int y = _y * iSideSize; y < ( _y + 1 ) * iSideSize; y++ )
        {
            CHex* pHex = GetHex( CHexCoord( x, y ) );
            if ( !avoidChangingWater || !pHex->IsWater( ) )
            {
                float edgeFactor = BlockEdgeFactor( x, y, _x, _y, iSideSize );
                float waveX       = sin( (float)x * 0.06f ) * 3.0f;
                float waveY       = cos( (float)y * 0.05f ) * 3.0f;
                int   waveDelta   = (int)( ( waveX + waveY ) * edgeFactor );
                pHex->SetAlt( pHex->GetAlt( ) + waveDelta );
            }
        }
    }*/   

    
    // Mesas / plateaus
    /*
    int iNumMesas = 2 + RandNum( 3 );
    for ( int iM = 0; iM < iNumMesas; iM++ )
    {
        int cx         = _x * iSideSize + RandNum( iSideSize );
        int cy         = _y * iSideSize + RandNum( iSideSize );
        int innerRad   = iSideSize / 12 + RandNum( iSideSize / 16 );
        int outerRad   = innerRad + iSideSize / 10 + RandNum( iSideSize / 12 );
        int mesaHeight = 15 + RandNum( 14 );

        for ( int x = cx - outerRad; x <= cx + outerRad; x++ )
        {
            for ( int y = cy - outerRad; y <= cy + outerRad; y++ )
            {
                int dx = x - cx;
                int dy = y - cy;
                int d  = (int)sqrt( (float)( dx * dx + dy * dy ) );
                if ( d > outerRad )
                    continue;

                CHex* pHex = GetHex( CHexCoord( x, y ) );
                if ( !pHex || (avoidChangingWater && pHex->IsWater( ) ))
                    continue;

                float edgeFactor = BlockEdgeFactor( x, y, _x, _y, iSideSize );
                if ( edgeFactor <= 0.0f || edgeFactor > 1.0f )
                    continue;

                int raise = 0;
                if ( d <= innerRad )
                    raise = mesaHeight;
                else
                {
                    float t = (float)( d - innerRad ) / (float)( outerRad - innerRad );
                    if ( t < 0.0f )
                        t = 0.0f;
                    if ( t > 1.0f )
                        t = 1.0f;
                    raise = (int)( ( 1.0f - t ) * (float)mesaHeight );
                }

                int delta = ConvertAlt( raise + CHex::sea_level, iSideSize ) - CHex::sea_level;

                delta     = (int)( (float)delta * (edgeFactor) );

                if ( delta > 0)
                {
                    delta = 0;
                  //  *edgeFactor;
                    if (delta > 10 || delta < 0)
                    {
                        delta = 10;
                    }
                    pHex->SetAlt( pHex->GetAlt( ) + delta );
                }
            }
        }
    }
    
    // Create sharp ridges and knife-edge features 
    
    int iNumRidges = 2 + RandNum( 3 );
    for ( int iRidge = 0; iRidge < iNumRidges; iRidge++ )
    {
        BOOL bHorizontal = ( MyRand( ) & 0x0100 ) != 0;
        int  ridgeStart  = _x * iSideSize + 4;
        int  ridgeEnd    = ( _x + 1 ) * iSideSize - 4;
        int  ridgePerp   = _y * iSideSize + RandNum( iSideSize - 8 ) + 4;

        if ( !bHorizontal )
        {
            ridgePerp  = _x * iSideSize + RandNum( iSideSize - 8 ) + 4;
            ridgeStart = _y * iSideSize + 4;
            ridgeEnd   = ( _y + 1 ) * iSideSize - 4;
        }

        int iNumSpires   = 2 + RandNum( 3 );
        int spireSpacing = ( ridgeEnd - ridgeStart ) / ( iNumSpires + 1 );

        for ( int iSpire = 0; iSpire < iNumSpires; iSpire++ )
        {
            int spirePos = ridgeStart + spireSpacing * ( iSpire + 1 ) + RandNum( spireSpacing / 3 ) - spireSpacing / 6;
            int xS, yS;
            if ( bHorizontal )
            {
                xS = spirePos;
                yS = ridgePerp + RandNum( 4 ) - 2;
            }
            else
            {
                xS = ridgePerp + RandNum( 4 ) - 2;
                yS = spirePos;
            }

            int spireBoost = 30 + RandNum( 35 );
            int maxRadius  = iSideSize / 6 + RandNum( iSideSize / 8 );

            for ( int radius = 0; radius <= maxRadius; radius++ )
            {
                float falloff = 1.0f - ( (float)radius / (float)maxRadius );
                falloff       = falloff * falloff;  // Broader mesas/ridges

                int currentBoost = (int)( spireBoost * falloff );
                if ( currentBoost <= 0 )
                    continue;

                for ( int angle = 0; angle < 360; angle += 15 )
                {
                    float rad = angle * 3.14159f / 180.0f;
                    int   rx  = xS + (int)( radius * cos( rad ) );
                    int   ry  = yS + (int)( radius * sin( rad ) );

                    CHex* pHex = GetHex( CHexCoord( rx, ry ) );
                    if ( !avoidChangingWater || !pHex->IsWater( ) )
                    {
                        float edgeFactor = BlockEdgeFactor( rx, ry, _x, _y, iSideSize );
                        int hexDelta =
                            ConvertAlt( currentBoost + RandNum( 4 ) + CHex::sea_level, iSideSize ) - CHex::sea_level;
                        pHex->SetAlt( pHex->GetAlt( ) + (int)( (float)hexDelta * edgeFactor ) );
                    }
                }
            }
        }
    }*/
    

    // Carve gullies and gorges (erosion)
    // Carve badlands terrain - gullies, buttes, and hoodoos
    int iNumGullies = 7 + RandNum( 4 );  // More gullies for heavily eroded look
    for ( int iG = 0; iG < iNumGullies; iG++ )
    {
        int length = iSideSize + RandNum( iSideSize / 2 );  // Longer, winding gullies
        int depth  = 10 + RandNum( 10 );
        int width  = 3 + RandNum( 5 );

        // Start near a random edge
        int edge = RandNum( 3 );
        int xG, yG;
        switch ( edge )
        {
        case 0:
            xG = _x * iSideSize + RandNum( iSideSize / 4 );
            yG = _y * iSideSize + RandNum( iSideSize );
            break;
        case 1:
            xG = _x * iSideSize + iSideSize - 1 - RandNum( iSideSize / 4 );
            yG = _y * iSideSize + RandNum( iSideSize );
            break;
        case 2:
            xG = _x * iSideSize + RandNum( iSideSize );
            yG = _y * iSideSize + RandNum( iSideSize / 4 );
            break;
        case 3:
            xG = _x * iSideSize + RandNum( iSideSize );
            yG = _y * iSideSize + iSideSize - 1 - RandNum( iSideSize / 4 );
            break;
        }

        int blockCenterX = _x * iSideSize + iSideSize / 2;
        int blockCenterY = _y * iSideSize + iSideSize / 2;

        int dxToCenter = blockCenterX - xG;
        int dyToCenter = blockCenterY - yG;

        int dir;
        if ( abs( dxToCenter ) > abs( dyToCenter ) * 2 )
            dir = ( dxToCenter > 0 ) ? 0 : 4;
        else if ( abs( dyToCenter ) > abs( dxToCenter ) * 2 )
            dir = ( dyToCenter > 0 ) ? 2 : 6;
        else if ( dxToCenter > 0 )
            dir = ( dyToCenter > 0 ) ? 1 : 7;
        else
            dir = ( dyToCenter > 0 ) ? 3 : 5;

        dir = ( dir + RandNum( 3 ) - 1 + 8 ) % 8;

        float currentDepth   = (float)depth * 0.3f;
        float depthIncrement = (float)depth * 0.7f / (float)length;
        float currentWidth   = (float)width * 0.7f;

        for ( int step = 0; step < length; step++ )
        {
            // More frequent direction changes - meandering erosion channels
            if ( RandNum( 3 ) == 0 )
                dir = ( dir + RandNum( 3 ) - 1 + 8 ) % 8;

            int dx = ( dir == 0 || dir == 1 || dir == 7 ) ? 1 : ( dir >= 3 && dir <= 5 ) ? -1 : 0;
            int dy = ( dir >= 1 && dir <= 3 ) ? 1 : ( dir >= 5 && dir <= 7 ) ? -1 : 0;
            xG += dx;
            yG += dy;

            currentDepth += depthIncrement + (float)( RandNum( 3 ) - 1 ) * 0.15f;
            currentWidth += 0.03f + (float)( RandNum( 3 ) - 1 ) * 0.02f;

            int stepWidth = (int)__min( currentWidth, (float)( width + 4 ) );
            int stepDepth = (int)__min( currentDepth, (float)( depth + 6 ) );

            for ( int wx = -stepWidth; wx <= stepWidth; wx++ )
            {
                for ( int wy = -stepWidth; wy <= stepWidth; wy++ )
                {
                    CHex* pHex = GetHex( CHexCoord( xG + wx, yG + wy ) );
                    if ( avoidChangingWater && pHex->IsWater( ) )
                        continue;

                    float dist = sqrtf( (float)( wx * wx + wy * wy ) );
                    if ( dist > (float)stepWidth )
                        continue;

                    float normalizedDist = dist / (float)stepWidth;

                    // Steeper V-shaped walls typical of water erosion in soft rock
                    float falloff = 1.0f - normalizedDist;
                    falloff       = falloff * falloff;  // Steeper sides

                    float noise = 1.0f + (float)( RandNum( 7 ) - 3 ) * 0.08f;

                    int carve = (int)( (float)stepDepth * falloff * noise );

                    if ( carve > 0 )
                    {
                        int newAlt = pHex->GetAlt( ) - carve;
                        newAlt     = min( 127, newAlt );
                        pHex->SetAlt( __max( 0, newAlt ) );
                    }
                }
            }

            // Branch gullies - smaller tributaries feeding main channel
            if ( RandNum( 12 ) == 0 )
            {
                int branchDir    = ( dir + ( RandNum( 2 ) == 0 ? 2 : -2 ) + 8 ) % 8;
                int branchLength = 5 + RandNum( 10 );
                int bx = xG, by = yG;
                int branchWidth = stepWidth / 2 + 1;
                int branchDepth = stepDepth / 2;

                for ( int bs = 0; bs < branchLength; bs++ )
                {
                    if ( RandNum( 4 ) == 0 )
                        branchDir = ( branchDir + RandNum( 3 ) - 1 + 8 ) % 8;

                    int bdx = ( branchDir == 0 || branchDir == 1 || branchDir == 7 ) ? 1
                              : ( branchDir >= 3 && branchDir <= 5 )                 ? -1
                                                                                     : 0;
                    int bdy = ( branchDir >= 1 && branchDir <= 3 ) ? 1 : ( branchDir >= 5 && branchDir <= 7 ) ? -1 : 0;
                    bx += bdx;
                    by += bdy;

                    // Branch gets shallower and narrower
                    branchDepth = __max( 2, branchDepth - 1 );
                    branchWidth = __max( 1, branchWidth - ( bs / 4 ) );

                    for ( int bwx = -branchWidth; bwx <= branchWidth; bwx++ )
                    {
                        for ( int bwy = -branchWidth; bwy <= branchWidth; bwy++ )
                        {
                            CHex* pHex = GetHex( CHexCoord( bx + bwx, by + bwy ) );
                            if ( avoidChangingWater && pHex->IsWater( ) )
                                continue;

                            float bdist = sqrtf( (float)( bwx * bwx + bwy * bwy ) );
                            if ( bdist > (float)branchWidth )
                                continue;

                            float bNormDist = bdist / (float)branchWidth;
                            float bFalloff  = 1.0f - bNormDist;
                            bFalloff        = bFalloff * bFalloff;

                            int bCarve = (int)( (float)branchDepth * bFalloff );
                            if ( bCarve > 0 )
                            {
                                int newAlt = pHex->GetAlt( ) - bCarve;
                                pHex->SetAlt( __max( 0, newAlt ) );
                            }
                        }
                    }
                }
            }
        }
    }

    // Create buttes and hoodoos - isolated pillars that resisted erosion
    int iNumButtes = 3 + RandNum( 4 );
    for ( int iB = 0; iB < iNumButtes; iB++ )
    {
        int bx = _x * iSideSize + iSideSize / 4 + RandNum( iSideSize / 2 );
        int by = _y * iSideSize + iSideSize / 4 + RandNum( iSideSize / 2 );

        int butteRadius = 3 + RandNum( 5 );
        int butteHeight = 10 + RandNum( 10 );

        // Flat-topped mesa or pointed hoodoo
        bool isMesa = RandNum( 3 ) == 0;

        for ( int bwx = -butteRadius - 2; bwx <= butteRadius + 2; bwx++ )
        {
            for ( int bwy = -butteRadius - 2; bwy <= butteRadius + 2; bwy++ )
            {
                CHex* pHex = GetHex( CHexCoord( bx + bwx, by + bwy ) );
                if ( avoidChangingWater && pHex->IsWater( ) )
                    continue;

                float dist = sqrtf( (float)( bwx * bwx + bwy * bwy ) );

                if ( dist <= (float)butteRadius )
                {
                    // Top of butte - raise it up
                    float normalizedDist = dist / (float)butteRadius;

                    int raise;
                    if ( isMesa )
                    {
                        // Flat top with steep edges
                        if ( normalizedDist < 0.7f )
                            raise = butteHeight;
                        else
                            raise = (int)( (float)butteHeight * ( 1.0f - normalizedDist ) / 0.3f );
                    }
                    else
                    {
                        // Hoodoo - pointed top, tapers up
                        float taper = 1.0f - ( normalizedDist * normalizedDist );
                        raise       = (int)( (float)butteHeight * taper );
                    }

                    // Add irregular edges
                    raise += RandNum( 3 ) - 1;

                    if ( raise > 0 )
                    {
                        int newAlt = pHex->GetAlt( ) + raise;
                        pHex->SetAlt( __min( 127, newAlt ) );
                    }
                }
                else if ( dist <= (float)( butteRadius + 2 ) )
                {
                    // Erosion debris at base - slight raise with rubble
                    int debris = RandNum( 3 );
                    if ( debris > 0 )
                    {
                        int newAlt = pHex->GetAlt( ) + debris;
                        pHex->SetAlt( __min( 127, newAlt ) );
                    }
                }
            }
        }
    }

    // Add overall surface roughness - wind erosion pitting
    for ( int rx = _x * iSideSize; rx < ( _x + 1 ) * iSideSize; rx++ )
    {
        for ( int ry = _y * iSideSize; ry < ( _y + 1 ) * iSideSize; ry++ )
        {
            if ( RandNum( 8 ) == 0 )  // Sparse pitting
            {
                CHex* pHex = GetHex( CHexCoord( rx, ry ) );
                if ( avoidChangingWater && pHex->IsWater( ) )
                    continue;

                // Small random height variation
                int variation = RandNum( 5 ) - 2;  // -2 to +2
                int newAlt    = pHex->GetAlt( ) + variation;
                pHex->SetAlt( __minmax( 0, 127, newAlt ) );
            }
        }
    }

    // Carve a few deep narrow slots - flash flood channels
    int iNumSlots = 1 + RandNum( 2 );
    for ( int iS = 0; iS < iNumSlots; iS++ )
    {
        int edge = RandNum( 3 );
        int sx, sy;
        switch ( edge )
        {
        case 0:
            sx = _x * iSideSize;
            sy = _y * iSideSize + iSideSize / 4 + RandNum( iSideSize / 2 );
            break;
        case 1:
            sx = _x * iSideSize + iSideSize - 1;
            sy = _y * iSideSize + iSideSize / 4 + RandNum( iSideSize / 2 );
            break;
        case 2:
            sx = _x * iSideSize + iSideSize / 4 + RandNum( iSideSize / 2 );
            sy = _y * iSideSize;
            break;
        case 3:
            sx = _x * iSideSize + iSideSize / 4 + RandNum( iSideSize / 2 );
            sy = _y * iSideSize + iSideSize - 1;
            break;
        }

        int blockCenterX = _x * iSideSize + iSideSize / 2;
        int blockCenterY = _y * iSideSize + iSideSize / 2;
        int dxToCenter   = blockCenterX - sx;
        int dyToCenter   = blockCenterY - sy;

        int dir;
        if ( abs( dxToCenter ) > abs( dyToCenter ) * 2 )
            dir = ( dxToCenter > 0 ) ? 0 : 4;
        else if ( abs( dyToCenter ) > abs( dxToCenter ) * 2 )
            dir = ( dyToCenter > 0 ) ? 2 : 6;
        else if ( dxToCenter > 0 )
            dir = ( dyToCenter > 0 ) ? 1 : 7;
        else
            dir = ( dyToCenter > 0 ) ? 3 : 5;

        int slotLength = iSideSize / 2 + RandNum( iSideSize / 2 );
        int slotDepth  = 18 + RandNum( 8 );  // Deep
        int slotWidth  = 1 + RandNum( 2 );   // Very narrow

        for ( int step = 0; step < slotLength; step++ )
        {
            // Slots are straighter - less meandering
            if ( RandNum( 6 ) == 0 )
                dir = ( dir + RandNum( 3 ) - 1 + 8 ) % 8;

            int dx = ( dir == 0 || dir == 1 || dir == 7 ) ? 1 : ( dir >= 3 && dir <= 5 ) ? -1 : 0;
            int dy = ( dir >= 1 && dir <= 3 ) ? 1 : ( dir >= 5 && dir <= 7 ) ? -1 : 0;
            sx += dx;
            sy += dy;

            for ( int swx = -slotWidth; swx <= slotWidth; swx++ )
            {
                for ( int swy = -slotWidth; swy <= slotWidth; swy++ )
                {
                    CHex* pHex = GetHex( CHexCoord( sx + swx, sy + swy ) );
                    if ( avoidChangingWater && pHex->IsWater( ) )
                        continue;

                    float dist = sqrtf( (float)( swx * swx + swy * swy ) );
                    if ( dist > (float)slotWidth )
                        continue;

                    // Nearly vertical walls
                    float normalizedDist = dist / (float)slotWidth;
                    float falloff        = ( normalizedDist < 0.6f ) ? 1.0f : ( 1.0f - normalizedDist ) / 0.4f;

                    int carve = (int)( (float)slotDepth * falloff );
                    if ( carve > 0 )
                    {
                        int newAlt = pHex->GetAlt( ) - carve;
                        pHex->SetAlt( __max( 0, newAlt ) );
                    }
                }
            }
        }
    }

    // (a stray `return;` used to sit here, making the slope-limit pass below
    // unreachable. Briefly resurrected 2026-06-10, then re-disabled on review:
    // spiky badlands are the desired look, and the pass is broken anyway —
    // ConvertAlt(5,...) is below sea_level so maxSlope comes out NEGATIVE,
    // which makes the clamp fire on every hex and invert terrain around the
    // local average instead of limiting it. Kept under #if 0 below.)

    /*
    // Soft smoothing pass to blend mesas and erosion
    {
        int baseX = _x * iSideSize;
        int baseY = _y * iSideSize;
        int count = iSideSize * iSideSize;
        int* pAlt = new int[count];

        for ( int pass = 0; pass < 3; pass++ )
        {
            for ( int x = baseX; x < baseX + iSideSize; x++ )
            {
                for ( int y = baseY; y < baseY + iSideSize; y++ )
                {
                    CHex* pHex = GetHex( CHexCoord( x, y ) );
                    int   idx  = ( x - baseX ) * iSideSize + ( y - baseY );
                    if ( pHex->IsWater( ) )
                    {
                        pAlt[idx] = pHex->GetAlt( );
                        continue;
                    }

                    int sumAlt = pHex->GetAlt( );
                    int cnt    = 1;

                    CHex* pN;
                    pN = GetHex( CHexCoord( x + 1, y ) );
                    if ( pN )
                    {
                        sumAlt += pN->GetAlt( );
                        cnt++;
                    }
                    pN = GetHex( CHexCoord( x - 1, y ) );
                    if ( pN )
                    {
                        sumAlt += pN->GetAlt( );
                        cnt++;
                    }
                    pN = GetHex( CHexCoord( x, y + 1 ) );
                    if ( pN )
                    {
                        sumAlt += pN->GetAlt( );
                        cnt++;
                    }
                    pN = GetHex( CHexCoord( x, y - 1 ) );
                    if ( pN )
                    {
                        sumAlt += pN->GetAlt( );
                        cnt++;
                    }
                    // Diagonals for smoother terraces
                    pN = GetHex( CHexCoord( x + 1, y + 1 ) );
                    if ( pN )
                    {
                        sumAlt += pN->GetAlt( );
                        cnt++;
                    }
                    pN = GetHex( CHexCoord( x - 1, y + 1 ) );
                    if ( pN )
                    {
                        sumAlt += pN->GetAlt( );
                        cnt++;
                    }
                    pN = GetHex( CHexCoord( x + 1, y - 1 ) );
                    if ( pN )
                    {
                        sumAlt += pN->GetAlt( );
                        cnt++;
                    }
                    pN = GetHex( CHexCoord( x - 1, y - 1 ) );
                    if ( pN )
                    {
                        sumAlt += pN->GetAlt( );
                        cnt++;
                    }

                    int avgAlt  = sumAlt / cnt;
                    int blended = ( pHex->GetAlt( ) + avgAlt * 2 ) / 3;
                    pAlt[idx]   = blended;
                }
            }

            for ( int x = baseX; x < baseX + iSideSize; x++ )
            {
                for ( int y = baseY; y < baseY + iSideSize; y++ )
                {
                    CHex* pHex = GetHex( CHexCoord( x, y ) );
                    if ( !pHex->IsWater( ) )
                    {
                        float edgeFactor = BlockEdgeFactor( x, y, _x, _y, iSideSize );
                        int   curAlt     = pHex->GetAlt( );
                        int   blendedAlt = pAlt[( x - baseX ) * iSideSize + ( y - baseY )];
                        pHex->SetAlt( curAlt + (int)( (float)( blendedAlt - curAlt ) * edgeFactor ) );
                    }
                }
            }
        }

        delete[] pAlt;
    }
    */


#if 0  // disabled: spikes are the badlands look, and maxSlope is negative (see note above)
    // Limit per-hex slope to avoid spiky badlands while keeping bold features
    {
        int baseX = _x * iSideSize;
        int baseY = _y * iSideSize;
        int maxSlope = ConvertAlt( 5, iSideSize ) - CHex::sea_level;

        for ( int x = baseX; x < baseX + iSideSize; x++ )
        {
            for ( int y = baseY; y < baseY + iSideSize; y++ )
            {
                CHex* pHex = GetHex( CHexCoord( x, y ) );
                if ( !pHex || pHex->IsWater( ) )
                    continue;

                int sumAlt = pHex->GetAlt( );
                int cnt    = 1;

                CHex* pN;
                pN = GetHex( CHexCoord( x + 1, y ) );
                if ( pN )
                {
                    sumAlt += pN->GetAlt( );
                    cnt++;
                }
                pN = GetHex( CHexCoord( x - 1, y ) );
                if ( pN )
                {
                    sumAlt += pN->GetAlt( );
                    cnt++;
                }
                pN = GetHex( CHexCoord( x, y + 1 ) );
                if ( pN )
                {
                    sumAlt += pN->GetAlt( );
                    cnt++;
                }
                pN = GetHex( CHexCoord( x, y - 1 ) );
                if ( pN )
                {
                    sumAlt += pN->GetAlt( );
                    cnt++;
                }

                int avgAlt = sumAlt / cnt;
                int curAlt = pHex->GetAlt( );
                if ( abs( curAlt - avgAlt ) > maxSlope )
                {
                    if ( curAlt > avgAlt )
                        pHex->SetAlt( avgAlt + maxSlope );
                    else
                        pHex->SetAlt( avgAlt - maxSlope );
                }
            }
        }
    }
#endif

}

// Gaussian-like blend weight calculation
// distance: distance from border (0 = on border, increases away)
// range: maximum blend distance (typically iSideSize/4 or iSideSize/5)
// returns: blend weight (0.0 = border, 1.0 = fully blended)
static float ComputeGaussianBlend( int distance, int range )
{
    if ( distance >= range )
        return 1.0f;
    if ( distance <= 0 )
        return 0.0f;

    // Gaussian-like curve: weight = 1 - exp(-(distance/range)^2)
    float normalized = (float)distance / (float)range;
    float weight     = 1.0f - exp( -normalized * normalized * 3.0f );  // 3.0f controls curve steepness
    return weight;
}

// Blend altitude smoothly across block borders using recursive subdivision
// Similar to InitSquarePass2 but focused on block boundaries
void CGameMap::SmoothBlockEdges( int iSideSize, int iSide )
{
    const int BLEND_WIDTH = iSideSize / 8;  // Width of blend zone on each side

    // Process horizontal borders (between top/bottom blocks)
    for ( int blockX = 0; blockX < iSide; ++blockX )
    {
        for ( int blockY = 0; blockY < iSide - 1; ++blockY )
        {
            int borderY = ( blockY + 1 ) * iSideSize;

            // Blend a strip above and below the border
            int yMin = borderY - BLEND_WIDTH;
            int yMax = borderY + BLEND_WIDTH;

            for ( int x = blockX * iSideSize; x < ( blockX + 1 ) * iSideSize; ++x )
            {
                for ( int y = yMin; y < yMax; ++y )
                {
                    CHex* pHexCurrent = GetHex( CHexCoord( x, y ) );
                    if ( !pHexCurrent || pHexCurrent->IsWater( ) )
                        continue;

                    // Calculate blend factor based on distance from border
                    float distFromBorder = (float)abs( y - borderY );
                    float blendFactor    = distFromBorder / (float)BLEND_WIDTH;
                    blendFactor          = blendFactor * blendFactor;  // Squared for smoother falloff

                    // Sample neighbors for smoothing
                    int altSum   = 0;
                    int altCount = 0;

                    for ( int dx = -1; dx <= 1; ++dx )
                    {
                        for ( int dy = -1; dy <= 1; ++dy )
                        {
                            CHex* pHexNeighbor = GetHex( CHexCoord( x + dx, y + dy ) );
                            if ( pHexNeighbor && !pHexNeighbor->IsWater( ) )
                            {
                                altSum += pHexNeighbor->GetAlt( );
                                altCount++;
                            }
                        }
                    }

                    if ( altCount > 0 )
                    {
                        int altAvg     = altSum / altCount;
                        int altCurrent = pHexCurrent->GetAlt( );

                        // Blend between current and average based on distance from border
                        int altNew = (int)( altCurrent * blendFactor + altAvg * ( 1.0f - blendFactor ) );

                        // Limit change to prevent spikes
                        if ( altNew < altCurrent - 10 )
                            altNew = altCurrent - 10;
                        else if ( altNew > altCurrent + 10 )
                            altNew = altCurrent + 10;

                        pHexCurrent->SetAlt( altNew );
                    }

                    // Blend terrain types near the border
                    if ( distFromBorder < BLEND_WIDTH / 2 )
                    {
                        // Find most common terrain type in neighbors
                        int terrainCounts[CHex::num_types] = { 0 };

                        for ( int dx = -2; dx <= 2; ++dx )
                        {
                            for ( int dy = -2; dy <= 2; ++dy )
                            {
                                CHex* pHexNeighbor = GetHex( CHexCoord( x + dx, y + dy ) );
                                if ( pHexNeighbor && !pHexNeighbor->IsWater( ) )
                                {
                                    int terrainType = pHexNeighbor->GetType( );
                                    if ( terrainType < CHex::num_types )
                                        terrainCounts[terrainType]++;
                                }
                            }
                        }

                        // Find most common terrain (excluding mountain)
                        int maxCount      = 0;
                        int commonTerrain = pHexCurrent->GetType( );
                        for ( int i = 0; i < CHex::num_types; ++i )
                        {
                            if ( i != CHex::mountain && terrainCounts[i] > maxCount )
                            {
                                maxCount      = terrainCounts[i];
                                commonTerrain = i;
                            }
                        }

                        // Probabilistically switch to common terrain
                        int switchChance = (int)( ( 1.0f - blendFactor ) * 100.0f );
                        if ( MyRand( ) % 100 < switchChance && commonTerrain != pHexCurrent->GetType( ) )
                        {
                            pHexCurrent->SetType( commonTerrain );
                        }
                    }
                }
            }
        }
    }

    // Process vertical borders (between left/right blocks)
    for ( int blockY = 0; blockY < iSide; ++blockY )
    {
        for ( int blockX = 0; blockX < iSide - 1; ++blockX )
        {
            int borderX = ( blockX + 1 ) * iSideSize;

            int xMin = borderX - BLEND_WIDTH;
            int xMax = borderX + BLEND_WIDTH;

            for ( int y = blockY * iSideSize; y < ( blockY + 1 ) * iSideSize; ++y )
            {
                for ( int x = xMin; x < xMax; ++x )
                {
                    CHex* pHexCurrent = GetHex( CHexCoord( x, y ) );
                    if ( !pHexCurrent || pHexCurrent->IsWater( ) )
                        continue;

                    float distFromBorder = (float)abs( x - borderX );
                    float blendFactor    = distFromBorder / (float)BLEND_WIDTH;
                    blendFactor          = blendFactor * blendFactor;

                    int altSum   = 0;
                    int altCount = 0;

                    for ( int dx = -1; dx <= 1; ++dx )
                    {
                        for ( int dy = -1; dy <= 1; ++dy )
                        {
                            CHex* pHexNeighbor = GetHex( CHexCoord( x + dx, y + dy ) );
                            if ( pHexNeighbor && !pHexNeighbor->IsWater( ) )
                            {
                                altSum += pHexNeighbor->GetAlt( );
                                altCount++;
                            }
                        }
                    }

                    if ( altCount > 0 )
                    {
                        int altAvg     = altSum / altCount;
                        int altCurrent = pHexCurrent->GetAlt( );

                        int altNew = (int)( altCurrent * blendFactor + altAvg * ( 1.0f - blendFactor ) );

                        if ( altNew < altCurrent - 10 )
                            altNew = altCurrent - 10;
                        else if ( altNew > altCurrent + 10 )
                            altNew = altCurrent + 10;

                        pHexCurrent->SetAlt( altNew );
                    }

                    if ( distFromBorder < BLEND_WIDTH && distFromBorder > 1 )
                    {
                        int terrainCounts[CHex::num_types] = { 0 };

                        for ( int dx = -2; dx <= 2; ++dx )
                        {
                            for ( int dy = -2; dy <= 2; ++dy )
                            {
                                CHex* pHexNeighbor = GetHex( CHexCoord( x + dx, y + dy ) );
                                if ( pHexNeighbor && !pHexNeighbor->IsWater( ) )
                                {
                                    int terrainType = pHexNeighbor->GetType( );
                                    if ( terrainType < CHex::num_types )
                                        terrainCounts[terrainType]++;
                                }
                            }
                        }

                        int maxCount      = 0;
                        int commonTerrain = pHexCurrent->GetType( );
                        for ( int i = 0; i < CHex::num_types; ++i )
                        {
                            if ( i != CHex::mountain && terrainCounts[i] > maxCount )
                            {
                                maxCount      = terrainCounts[i];
                                commonTerrain = i;
                            }
                        }

                        int switchChance = (int)( ( 1.0f - blendFactor ) * 100.0f );
                        if ( MyRand( ) % 100 < switchChance && commonTerrain != pHexCurrent->GetType( ) )
                        {
                            pHexCurrent->SetType( commonTerrain );
                        }
                    }
                }
            }
        }
    }
}