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

void CGameMap::GenerateOcean( int iNumBlks, int* piBlks, int iSide, int blockType, int& iOceansLeft, 
    CGame& theGame )
{
    {
        int maxOceanTileCount = iNumBlks - theGame.GetAll( ).GetCount( ) * ( ( iNumBlks > 32 ) ? 3 : 2 );

        if ( maxOceanTileCount <= 0 )
            maxOceanTileCount = iNumBlks - theGame.GetAll( ).GetCount( );

        if ( maxOceanTileCount > 0 )
        {
            // somewhere between no ocean, and a huge ocean, but try to keep ~20% + player starts of it free
            int totalCount = MyRand( ) % ( maxOceanTileCount );

            int oceanStyle = MyRand( ) % 4;  // 0 = stripe, 1 = scatter, 2 grow, grow+island

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
        else
        {
            // no ocean tiles for us :(
            TRAP( );
        }
    }
}

void CGameMap::GenerateMountainBlock( int _x, int iSideSize, int _y, const int iTry2[10] )
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
                int newAlt = __min( CHex::MaxAlt, ConvertAlt( baseAlt, iSideSize ) );
                pHexPeak->SetAlt( newAlt );
                pHexPeak->SetType( CHex::mountain );

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
                        float rad    = angle * 3.14159f / 180.0f;
                        // Add jaggedness to the ring
                        int   jitter = RandNum( 3 ) - 1;
                        int   rx     = xPeak + (int)( ( radius + jitter ) * cos( rad ) );
                        int   ry     = yPeak + (int)( ( radius + jitter ) * sin( rad ) );

                        CHex* pHex = GetHex( CHexCoord( rx, ry ) );
                        if ( !pHex->IsWater( ) )
                        {
                            int existingAlt = pHex->GetAlt( );
                            int proposedAlt = __min( CHex::MaxAlt, ConvertAlt( ringAlt + RandNum( 8 ), iSideSize ) );

                            // Only raise terrain, don't lower existing peaks
                            if ( proposedAlt > existingAlt )
                            {
                                pHex->SetAlt( proposedAlt );

                                // Set terrain type based on altitude
                                if ( ringAlt > 70 )
                                    pHex->SetType( CHex::mountain );
                                else if ( ringAlt > 50 )
                                    pHex->SetType( CHex::hill );
                                else
                                    pHex->SetType( CHex::rough );
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
                            pHex->SetAlt( proposedAlt );
                            if ( hexAlt > 60 )
                                pHex->SetType( CHex::mountain );
                            else
                                pHex->SetType( CHex::hill );
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
                            pHex->SetAlt( ConvertAlt( bowlAlt, iSideSize ) );

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

                // Gentle meander (glaciers don't wind as much as rivers)
                float meander = sin( t * 4.0f ) * ( iSideSize / 20.0f );

                int baseX = vx1 + (int)( ( vx2 - vx1 ) * t );
                int baseY = vy1 + (int)( ( vy2 - vy1 ) * t );

                if ( bHorizontal )
                    baseY += (int)meander;
                else
                    baseX += (int)meander;

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

                        // Valley carves into terrain (can lower it)
                        if ( absW <= valleyWidth || proposedAlt < pHex->GetAlt( ) )
                        {
                            pHex->SetAlt( proposedAlt );
                        }
                    }
                }
            }
        }
    }

    // Add alpine forest on lower slopes
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
    MakeMineral( _x * iSideSize + 4 + RandNum( iSideSize - 8 ), _y * iSideSize + 1 + RandNum( iSideSize - 8 ),
                 CMaterialTypes::coal, iSideSize );
    MakeMineral( _x * iSideSize + 4 + RandNum( iSideSize - 8 ), _y * iSideSize + 1 + RandNum( iSideSize - 8 ),
                 CMaterialTypes::iron, iSideSize / 2 );

    // Add varied terrain in lower areas
    xDrop = _x * iSideSize + 8 + RandNum( iSideSize - 16 );
    yDrop = _y * iSideSize + 8 + RandNum( iSideSize - 16 );
    MakeTerrain( xDrop, yDrop, iTry2[RandNum( 8 )], iSideSize / 2 );
}
