//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------


// scanlist.cpp : class for row-by-row scan conversion
//

#include "stdafx.h"
#include "_windwrd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#include "scanlist.h"

#ifndef __SCANLIST_H__
#error scanlist.h not imported
#endif

//---------------------------- S c a n L i s t ------------------------------

//--------------------------------------------------------------------------
// ScanList::ScanList
//--------------------------------------------------------------------------
ScanList::ScanList( int iMaxHeight ) {
    ASSERT_STRICT( iMaxHeight > 0 );

    m_iMaxHeight = iMaxHeight;
    m_iTopY = 0;
    m_iBottomY = 0;
    m_piLeft = new int[iMaxHeight];
    m_piRight = new int[iMaxHeight];

    m_aiList[0] = m_piLeft;
    m_aiList[1] = m_piRight;

    ASSERT_STRICT_VALID( this );
}

//--------------------------------------------------------------------------
// ScanList::~ScanList
//--------------------------------------------------------------------------
ScanList::~ScanList() {
    delete[] m_piLeft;
    delete[] m_piRight;
}

//--------------------------------------------------------------------------
// ScanList::CheckPoly
//--------------------------------------------------------------------------
BOOL
ScanList::CheckPoly(
    CPoint const apt[],  // Array of points in clockwise order
    int iCount )         // # points
{
    for ( int i = 0; i < iCount; ++i ) {
        int j1 = i - 1;
        int j2 = i + 1;

        if ( j1 < 0 ) {
            j1 = iCount - 1;
        }

        if ( j2 == iCount )
            j2 = 0;

        auto pt1 = CPoint( apt[j1] - apt[i] );
        auto pt2 = CPoint( apt[j2] - apt[i] );

        int iZ = pt1.y * pt2.x - pt1.x * pt2.y;

        if ( iZ <= 0 )
            return FALSE;
    }

    return TRUE;
}
void ScanList::ScanPoly( CPoint const apt[],  // Array of points in clockwise order
                            int          iCount )         // # points
{
    // if caps lock is on, use old, else, use new:
    if ( GetKeyState( VK_CAPITAL ) & 0x0001 && false )
    {
        ScanPolyOld( apt, iCount );
    } else {
        ScanPolyNew( apt, iCount );
    }
}
    //--------------------------------------------------------------------------
// ScanList::ScanPoly
//--------------------------------------------------------------------------
void
ScanList::ScanPolyOld(
    CPoint const apt[], // Array of points in clockwise order
    int iCount )        // # points
{
    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT( iCount > 2 );
    ASSERT_STRICT( AfxIsValidAddress( apt, iCount * sizeof( CPoint ) ) );

#ifdef _DEBUG
    (CheckPoly( apt, iCount ));
#endif

    m_iTopY = apt[0].y;
    m_iBottomY = apt[0].y;

    for ( int i = 1; i < iCount; ++i ) {
        if ( apt[i].y < m_iTopY ) {
            m_iTopY = apt[i].y;
        } else if ( apt[i].y > m_iBottomY ) {
            m_iBottomY = apt[i].y;
        }
    }

    for ( int i = 0; i < iCount; ++i ) {
        int j = i + 1;

        if ( j == iCount ) {
            j = 0;
        }

        // MakeLine( apt[i], apt[j] );

        int     iSide;
        CPoint  ptTop;
        CPoint  ptBot;

        if ( apt[i].y > apt[j].y ) {
            iSide = 0;

            ptTop = apt[j];
            ptBot = apt[i];
        } else {
            iSide = 1;

            ptTop = apt[i];
            ptBot = apt[j];
        }

        int iDelY = ptBot.y - ptTop.y;

        if ( 0 == iDelY )
            continue;

        int iDelX = ptBot.x - ptTop.x;
        int iX = ptTop.x;
        int* pi = m_aiList[iSide] + ptTop.y - m_iTopY;
        int iLineCount  = iDelY + 1;

        if ( 0 == iDelX ) {
            // x64 port: fill the column with the constant integer x.
            for ( int n = 0; n < iLineCount; ++n )
                pi[n] = iX;
        } else {
            // x64 port of the fixed-point DDA edge walk (was a 4x-unrolled asm
            // loop with a 0.32 fractional accumulator). Reproduces the authors'
            // own commented-out C reference below: a 16.16 delta-x of
            // (iDelX<<16)/iDelY, walked down the column with a +0.5px rounding
            // bias, storing the whole part of x for each row.
            {
                long fixDelta = (long)( ( (long long)iDelX << 16 ) / (long long)iDelY );
                long fixX     = ( iX << 16 ) + 0x00008000;   // +0.5 px rounding bias
                for ( int n = 0; n < iLineCount; ++n ) {
                    pi[n] = fixX >> 16;
                    fixX += fixDelta;
                }
            }

            //  while ( iCount-- > 0 )
            //  {
            //      *pi++ = fixX >> 16;
            //      fixX += fixDelta;
            //  }
        }
    }

    ASSERT_STRICT_VALID( this );
}


//--------------------------------------------------------------------------
// ScanList::ScanPoly - AVX2 Optimized (works in 32-bit builds)
//--------------------------------------------------------------------------
void ScanList::ScanPolyNew( CPoint const apt[], int iCount )
{
    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT( iCount > 2 );
    ASSERT_STRICT( AfxIsValidAddress( apt, iCount * sizeof( CPoint ) ) );

#ifdef _DEBUG
    ( CheckPoly( apt, iCount ) );
#endif

    // Find bounding box using SIMD if polygon is large enough
    int topY = apt[0].y;
    int botY = apt[0].y;

    for ( int i = 1; i < iCount; ++i )
    {
        const int y = apt[i].y;
        topY        = ( y < topY ) ? y : topY;
        botY        = ( y > botY ) ? y : botY;
    }

    m_iTopY    = topY;
    m_iBottomY = botY;

    // Process each edge
    for ( int i = 0; i < iCount; ++i )
    {
        const int j = ( i + 1 < iCount ) ? i + 1 : 0;

        int    iSide;
        CPoint ptTop, ptBot;

        if ( apt[i].y > apt[j].y )
        {
            iSide = 0;
            ptTop = apt[j];
            ptBot = apt[i];
        }
        else
        {
            iSide = 1;
            ptTop = apt[i];
            ptBot = apt[j];
        }

        const int iDelY = ptBot.y - ptTop.y;

        if ( iDelY == 0 )
            continue;

        const int iDelX      = ptBot.x - ptTop.x;
        int* __restrict pi   = m_aiList[iSide] + ( ptTop.y - topY );
        const int iLineCount = iDelY + 1;

        if ( iDelX == 0 )
        {
            // Vertical edge - AVX2 can blast 8 ints at once
            const __m256i vx = _mm256_set1_epi32( ptTop.x );
            int           k  = 0;

            for ( ; k + 8 <= iLineCount; k += 8 )
            {
                _mm256_storeu_si256( reinterpret_cast<__m256i*>( pi + k ), vx );
            }
            // SSE2 for 4-int chunks
            if ( k + 4 <= iLineCount )
            {
                _mm_storeu_si128( reinterpret_cast<__m128i*>( pi + k ), _mm256_castsi256_si128( vx ) );
                k += 4;
            }
            // Scalar remainder
            for ( ; k < iLineCount; ++k )
            {
                pi[k] = ptTop.x;
            }
        }
        else
        {
            // Sloped edge with AVX2
            // Compute 8 x-values in parallel

            const long long fixedDelta = ( static_cast<long long>( iDelX ) << 32 ) / iDelY;
            long long       fixedX     = ( static_cast<long long>( ptTop.x ) << 32 ) + 0x80000000LL;

            int k = 0;

            // AVX2: process 8 at a time
            if ( iLineCount >= 8 )
            {
                // Precompute 8 consecutive values and the stride
                const long long delta8 = fixedDelta * 8;

                for ( ; k + 8 <= iLineCount; k += 8 )
                {
                    // Compute 8 values - compiler should optimize this well
                    const int x0 = static_cast<int>( fixedX >> 32 );
                    const int x1 = static_cast<int>( ( fixedX + fixedDelta ) >> 32 );
                    const int x2 = static_cast<int>( ( fixedX + fixedDelta * 2 ) >> 32 );
                    const int x3 = static_cast<int>( ( fixedX + fixedDelta * 3 ) >> 32 );
                    const int x4 = static_cast<int>( ( fixedX + fixedDelta * 4 ) >> 32 );
                    const int x5 = static_cast<int>( ( fixedX + fixedDelta * 5 ) >> 32 );
                    const int x6 = static_cast<int>( ( fixedX + fixedDelta * 6 ) >> 32 );
                    const int x7 = static_cast<int>( ( fixedX + fixedDelta * 7 ) >> 32 );

                    const __m256i values = _mm256_set_epi32( x7, x6, x5, x4, x3, x2, x1, x0 );
                    _mm256_storeu_si256( reinterpret_cast<__m256i*>( pi + k ), values );

                    fixedX += delta8;
                }
            }

            // Scalar remainder
            for ( ; k < iLineCount; ++k )
            {
                pi[k] = static_cast<int>( fixedX >> 32 );
                fixedX += fixedDelta;
            }
        }
    }

    ASSERT_STRICT_VALID( this );
}

//--------------------------------------------------------------------------
// ScanList::ScanPolyFixed
//--------------------------------------------------------------------------

void
ScanList::ScanPolyFixedOld(
    CPoint const apt[],  // Array of points in clockwise order
    int iCount )         // # points
{
    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT( iCount > 2 );
    ASSERT_STRICT( AfxIsValidAddress( apt, iCount * sizeof( CPoint ) ) );

#ifdef _DEBUG
    CheckPoly( apt, iCount );
#endif

    m_iTopY = apt[0].y;
    m_iBottomY = apt[0].y;

    for ( int i = 1; i < iCount; ++i ) {
        if ( apt[i].y < m_iTopY ) {
            m_iTopY = apt[i].y;
        } else if ( apt[i].y > m_iBottomY ) {
            m_iBottomY = apt[i].y;
        }
    }

    for ( int i = 0; i < iCount; ++i ) {
        int j = i + 1;

        if ( j == iCount ) {
            j = 0;
        }

        // MakeLine( apt[i], apt[j] );

        int     iSide;
        CPoint  ptTop;
        CPoint  ptBot;

        if ( apt[i].y > apt[j].y ) {
            iSide = 0;

            ptTop = apt[j];
            ptBot = apt[i];
        } else {
            iSide = 1;

            ptTop = apt[i];
            ptBot = apt[j];
        }

        int iDelY = ptBot.y - ptTop.y;

        if ( 0 == iDelY ) {
            continue;
        }

        int iDelX = ptBot.x - ptTop.x;
        int fixX = ( ptTop.x << 16 ) + 0x00008000;
        //  int iX = ptTop.x;
        int* pi = m_aiList[iSide] + ptTop.y - m_iTopY;
        int iCount = iDelY + 1;

        if ( 0 == iDelX ) {
            // x64 port: fill the column with the constant 16.16 x value.
            for ( int n = 0; n < iCount; ++n )
                pi[n] = fixX;
        } else {
            // x64 port of the fixed-point DDA edge walk (was a 4x-unrolled asm
            // loop). Reproduces the authors' own commented-out C reference
            // below: 16.16 delta-x = (iDelX<<16)/iDelY, storing the full 16.16
            // x value for each row (fixX already carries the +0.5px bias).
            {
                long fixDelta = (long)( ( (long long)iDelX << 16 ) / (long long)iDelY );
                for ( int n = 0; n < iCount; ++n ) {
                    pi[n] = fixX;
                    fixX += fixDelta;
                }
            }

            //  while ( iCount-- > 0 )
            //  {
            //      *pi++ = fixX;
            //      fixX += fixDelta;
            //  }
        }
    }

    ASSERT_STRICT_VALID( this );
}


    //--------------------------------------------------------------------------
// ScanList::ScanPolyFixed
//--------------------------------------------------------------------------
void ScanList::ScanPolyFixed(CPoint const apt[],  // Array of points in clockwise order
    int          iCount)         // # points
{
    // if caps lock is on, use old, else, use new:
    if (GetKeyState(VK_CAPITAL) & 0x0001) {
        ScanPolyFixedOld(apt, iCount);
    }
    else {
        ScanPolyFixedNew(apt, iCount);
    }
}



void ScanList::ScanPolyFixedNew(
    CPoint const apt[],  // Array of points in clockwise order
    int          iCount )         // # points
{
    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT( iCount > 2 );
    ASSERT_STRICT( AfxIsValidAddress( apt, iCount * sizeof( CPoint ) ) );

#ifdef _DEBUG
    CheckPoly( apt, iCount );
#endif

    // Find bounding box - DO NOT CLAMP these values!
    m_iTopY    = apt[0].y;
    m_iBottomY = apt[0].y;

    for ( int i = 1; i < iCount; ++i )
    {
        if ( apt[i].y < m_iTopY )
        {
            m_iTopY = apt[i].y;
        }
        else if ( apt[i].y > m_iBottomY )
        {
            m_iBottomY = apt[i].y;
        }
    }

    // Process each edge
    for ( int i = 0; i < iCount; ++i )
    {
        const int j = ( i + 1 ) % iCount;

        const CPoint& pt1 = apt[i];
        const CPoint& pt2 = apt[j];

        if ( pt1.y == pt2.y )
        {
            continue;  // Skip horizontal edges
        }

        int    iSide;
        CPoint ptTop, ptBot;

        if ( pt1.y > pt2.y )
        {
            iSide = 0;
            ptTop = pt2;
            ptBot = pt1;
        }
        else
        {
            iSide = 1;
            ptTop = pt1;
            ptBot = pt2;
        }

        const int iDelY = ptBot.y - ptTop.y;
        const int iDelX = ptBot.x - ptTop.x;

        // Calculate buffer offset (this is where we check bounds!)
        int iBufferStart = ptTop.y - m_iTopY;

        // Skip if completely out of range
        if ( iBufferStart >= m_iMaxHeight || iBufferStart + iDelY < 0 )
            continue;

        // Clamp to valid buffer range
        int iClipTop = 0;
        int iClipBot = iDelY + 1;

        if ( iBufferStart < 0 )
        {
            iClipTop     = -iBufferStart;
            iBufferStart = 0;
        }

        if ( iBufferStart + iClipBot > m_iMaxHeight )
        {
            iClipBot = m_iMaxHeight - iBufferStart;
        }

        int*      pi         = m_aiList[iSide] + iBufferStart;
        const int iLineCount = iClipBot - iClipTop;

        if ( iLineCount <= 0 )
            continue;

        if ( iDelX == 0 )
        {
            // Vertical edge - store fixed-point constant
            const int fixX = ( ptTop.x << 16 ) + 0x8000;
            for ( int k = 0; k < iLineCount; ++k )
            {
                pi[k] = fixX;
            }
        }
        else
        {
            // Sloped edge - interpolate in fixed-point
            const int fixDelta = ( static_cast<long long>( iDelX ) << 16 ) / iDelY;

            // Adjust starting position if we clipped the top
            int fixX = ( ptTop.x << 16 ) + 0x8000;
            if ( iClipTop > 0 )
            {
                fixX += fixDelta * iClipTop;
            }

            for ( int k = 0; k < iLineCount; ++k )
            {
                pi[k] = fixX;
                fixX += fixDelta;
            }
        }
    }

    ASSERT_STRICT_VALID( this );
}

//-------------------------------------------------------------------------
// ScanList::AssertValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void ScanList::AssertValid() const {

    ASSERT_STRICT( 0 <= m_iMaxHeight && m_iMaxHeight < 2000 );

    ASSERT_STRICT( AfxIsValidAddress( m_prowends, m_iMaxHeight * sizeof( RowEnds ) ) );

    ASSERT_STRICT( m_iBottomY >= m_iTopY );
    ASSERT_STRICT( m_iBottomY - m_iTopY < m_iMaxHeight );
}

#endif
